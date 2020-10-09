// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#include "BackgroundEncoder.h"

#include <algorithm>

BackgroundEncoder::ByteArray BackgroundEncoder::TrainingData::Serialise() const
{
	BitPacker bit_packer;

	PrefixEncoding encoder{256, std::vector<std::uint64_t>(256)};

	auto serialise_frequencies = [&](const auto& frequencies) {
		encoder.EncodeUint64Varint(frequencies.size(), bit_packer, false);
		for (auto freq : frequencies)
		{
			encoder.EncodeUint64Varint(freq, bit_packer, false);
		}
	};

	serialise_frequencies(entity_number_frequencies);
	serialise_frequencies(entity_id_diff_frequencies);
	serialise_frequencies(common_value_diff_index_frequencies);
	serialise_frequencies(value_frequencies);

	// Encode length-delimited common value diffs.
	encoder.EncodeUint64Varint(common_value_diffs.size(), bit_packer, false);
	for (auto& common_value_diff : common_value_diffs)
	{
		encoder.EncodeUint64Varint(common_value_diff.diff.size(), bit_packer, false);
		for (auto v : common_value_diff.diff)
		{
			encoder.EncodeUint64Varint(v, bit_packer, false);
		}
		encoder.EncodeUint64Varint(common_value_diff.next_diff_indices.size(), bit_packer, false);
		for (auto i : common_value_diff.next_diff_indices)
		{
			encoder.EncodeUint64Varint(i, bit_packer, false);
		}
	}

	ByteArray output;
	bit_packer.WriteToBuffer(output);
	return output;
}

BackgroundEncoder::TrainingData BackgroundEncoder::TrainingData::Deserialise(const ByteArray& serialised)
{
	BitUnpacker bit_unpacker{serialised};

	PrefixEncoding encoder{256, std::vector<std::uint64_t>(256)};

	auto deserialise_frequencies = [&](auto& frequencies) {
		frequencies.resize(encoder.DecodeUint64Varint(bit_unpacker, false));
		for (auto i = 0; i < frequencies.size(); i++)
		{
			frequencies[i] = encoder.DecodeUint64Varint(bit_unpacker, false);
		}
	};

	TrainingData training_data{};
	deserialise_frequencies(training_data.entity_number_frequencies);
	deserialise_frequencies(training_data.entity_id_diff_frequencies);
	deserialise_frequencies(training_data.common_value_diff_index_frequencies);
	deserialise_frequencies(training_data.value_frequencies);

	// Decode length-delimited common value diffs.
	training_data.common_value_diffs.resize(encoder.DecodeUint64Varint(bit_unpacker, false));
	for (auto i = 0; i < training_data.common_value_diffs.size(); i++)
	{
		training_data.common_value_diffs[i].diff.resize(encoder.DecodeUint64Varint(bit_unpacker, false));
		for (auto v = 0; v < training_data.common_value_diffs[i].diff.size(); v++)
		{
			training_data.common_value_diffs[i].diff[v] = encoder.DecodeUint64Varint(bit_unpacker, false);
		}

		training_data.common_value_diffs[i].next_diff_indices.resize(encoder.DecodeUint64Varint(bit_unpacker, false));
		for (auto v = 0; v < training_data.common_value_diffs[i].next_diff_indices.size(); v++)
		{
			training_data.common_value_diffs[i].next_diff_indices[v] = encoder.DecodeUint64Varint(bit_unpacker, false);
		}
	}

	return training_data;
}

BackgroundEncoder::BackgroundEncoder(
	std::size_t num_values, std::size_t num_next_diff_elements, std::size_t num_dictionary_elements)
	: num_values{num_values}
	, num_next_diff_elements{num_next_diff_elements}
	, num_dictionary_elements{num_dictionary_elements - 1 - num_next_diff_elements}
	, entity_number_encoder{256, std::vector<std::uint64_t>(256)}
	, entity_id_diff_encoder{256, std::vector<std::uint64_t>(256)}
	, common_value_diff_index_encoder{this->num_dictionary_elements + 1 + num_next_diff_elements,
		  std::vector<std::uint64_t>(this->num_dictionary_elements + 1 + num_next_diff_elements)}
	, value_encoder{256, std::vector<std::uint64_t>(256)}
{
}

std::size_t BackgroundEncoder::GetNumValuesPerEntity() const
{
	return num_values;
}

std::size_t BackgroundEncoder::GetNumEntities() const
{
	return entities.size();
}

std::vector<BackgroundEncoder::EntityId> BackgroundEncoder::GetEntityIds() const
{
	std::vector<EntityId> entity_ids;
	entity_ids.reserve(GetNumEntities());
	for (auto& kvp : entities)
	{
		entity_ids.push_back(kvp.first);
	}
	return entity_ids;
}

const BackgroundEncoder::Values* BackgroundEncoder::GetEntityValues(EntityId entity_id) const
{
	auto it = entities.find(entity_id);
	if (it == entities.end())
	{
		return nullptr;
	}
	else
	{
		return &it->second.current_entity_values;
	}
}

BackgroundEncoder::ByteArray BackgroundEncoder::CreateInitialData()
{
	BitPacker bit_packer;

	for (auto& kvp : entities)
	{
		auto entity_id = kvp.first;
		auto& entity_state = kvp.second;

		// Encode entity ID.
		assert(entity_id > 0);
		entity_id_diff_encoder.EncodeUint64Varint(entity_id, bit_packer, false);

		// Encode current values.
		assert(entity_state.current_entity_values.size() == num_values);
		for (auto v = 0; v < num_values; v++)
		{
			value_encoder.EncodeUint64Varint(entity_state.current_entity_values[v], bit_packer, false);
		}

		// Encode last diff.
		for (auto v = 0; v < num_values; v++)
		{
			value_encoder.EncodeUint64Varint(entity_state.last_entity_diff[v], bit_packer, false);
		}
	}

	// Terminate with 0 entity ID.
	entity_id_diff_encoder.EncodeUint64Varint(0, bit_packer, false);

	ByteArray output_buffer;
	bit_packer.WriteToBuffer(output_buffer);
	return output_buffer;
}

void BackgroundEncoder::ApplyInitialData(const ByteArray& initial_data)
{
	BitUnpacker bit_unpacker{initial_data};

	entities.clear();

	while (bit_unpacker.BitsRemaining() > 0)
	{
		auto entity_id = entity_id_diff_encoder.DecodeUint64Varint(bit_unpacker, false);
		if (entity_id == 0)
		{
			// End of entities.
			break;
		}

		Values current_entity_values(num_values);
		for (auto v = 0; v < num_values; v++)
		{
			current_entity_values[v] = value_encoder.DecodeUint64Varint(bit_unpacker, false);
		}

		Values last_entity_diff(num_values);
		for (auto v = 0; v < num_values; v++)
		{
			last_entity_diff[v] = value_encoder.DecodeUint64Varint(bit_unpacker, false);
		}

		entities.insert(
			{entity_id, EntityState{Values(num_values), std::move(last_entity_diff), std::move(current_entity_values)}});
	}
}

BackgroundEncoder::ByteArray BackgroundEncoder::FlushNetworkDelta()
{
	BitPacker bit_packer;

	std::map<EntityId, bool> entity_id_set_diff;
	// Find new entities.
	for (auto& kvp : entities)
	{
		if (last_flushed_entity_id_set.count(kvp.first) == 0)
		{
			// This entity was not in the last flushed entity set.
			entity_id_set_diff[kvp.first] = true;
			last_flushed_entity_id_set.insert(kvp.first);
		}
	}

	// Find deleted entities.
	for (auto entity_id : entities_deleted_since_flush)
	{
		if (last_flushed_entity_id_set.count(entity_id) > 0)
		{
			// This entity was in the last flushed and is now deleted.
			entity_id_set_diff[entity_id] = false;
			last_flushed_entity_id_set.erase(entity_id);
			entities.erase(entity_id);
		}
	}

	entities_deleted_since_flush.clear();

	// Send entity ID set diff.
	// entity_id_set_diff must iterate in ascending order!
	EntityId last_entity_id = 0;
	for (auto& kvp : entity_id_set_diff)
	{
		auto entity_id = kvp.first;
		auto entity_id_diff = entity_id - last_entity_id;
		assert(entity_id_diff > 0);
		entity_id_diff_encoder.EncodeUint64Varint(entity_id_diff, bit_packer, true);
		// 1 is new entity, 0 is deleted entity.
		bit_packer.WriteBack(kvp.second ? 0b1 : 0b0, 1);
		last_entity_id = entity_id;
	}
	// Terminate with 0 entity diff.
	entity_id_diff_encoder.EncodeUint64Varint(0, bit_packer, true);

	// Encode each entities index.
	// entities must iterate in ascending order!
	Values entity_diff;
	for (auto& kvp : entities)
	{
		auto entity_id = kvp.first;
		auto& entity_state = kvp.second;

		// Calculate the entity diff.
		assert(entity_state.last_entity_values.size() == entity_state.current_entity_values.size());
		entity_diff.resize(entity_state.last_entity_values.size());
		for (auto v = 0; v < entity_state.last_entity_values.size(); v++)
		{
			auto last_value = entity_state.last_entity_values[v];
			auto current_value = entity_state.current_entity_values[v];
			auto diff = current_value - last_value;
			auto zigzag_diff = ZigZagEncode(diff);
			entity_diff[v] = zigzag_diff;
			entity_state.last_entity_values[v] = current_value;
		}

		// Count the entity diff for training.
		value_frequencies[entity_diff]++;
		next_diff_frequencies[entity_state.last_entity_diff][entity_diff]++;

		// See if diff is in the dictionary.
		auto common_value_it = common_value_diff_indices.find(entity_diff);
		if (common_value_it == common_value_diff_indices.end())
		{
			// This entity diff is not in the dictionary, encode control 0 and then
			// values.
			common_value_diff_index_encoder.EncodeSymbol(0, bit_packer, true);

			for (auto value_diff : entity_diff)
			{
				value_encoder.EncodeUint64Varint(value_diff, bit_packer, true);
			}
		}
		else
		{
			// This entity diff is in the dictionary, encode index.
			auto common_value_diff_index = common_value_it->second;

			auto last_entity_diff_it = common_value_diff_indices.find(entity_state.last_entity_diff);
			if (last_entity_diff_it != common_value_diff_indices.end())
			{
				bool has_encoded = false;
				auto& last_next_diff_indices_list = common_value_diffs[last_entity_diff_it->second].next_diff_indices;
				for (auto index = 0; index < last_next_diff_indices_list.size(); index++)
				{
					auto value_diff_index = last_next_diff_indices_list[index];
					if (value_diff_index == common_value_diff_index)
					{
						// The last entity diff for this entity has an entry in its
						// next_diff_indices for this new entity diff. Encode this index
						// into next_diff_indices.
						common_value_diff_index_encoder.EncodeSymbol(1 + index, bit_packer, true);
						has_encoded = true;
						break;
					}
				}

				if (!has_encoded)
				{
					common_value_diff_index_encoder.EncodeSymbol(
						1 + last_next_diff_indices_list.size() + common_value_diff_index, bit_packer, true);
				}
			}
			else
			{
				common_value_diff_index_encoder.EncodeSymbol(1 + common_value_diff_index, bit_packer, true);
			}
		}

		entity_state.last_entity_diff = entity_diff;
	}

	ByteArray output_buffer;
	bit_packer.WriteToBuffer(output_buffer);
	return output_buffer;
}

void BackgroundEncoder::ApplyNetworkDelta(const ByteArray& network_delta)
{
	BitUnpacker bit_unpacker{network_delta};

	// Decode entity ID set diff.
	EntityId last_entity_id = 0;
	while (bit_unpacker.BitsRemaining() > 0)
	{
		auto entity_id_diff = entity_id_diff_encoder.DecodeUint64Varint(bit_unpacker, false);
		if (entity_id_diff == 0)
		{
			// End of entities.
			break;
		}

		auto entity_id = last_entity_id + entity_id_diff;
		last_entity_id = entity_id;

		bool is_added = bit_unpacker.ReadSymbol(1) != 0b0;
		if (is_added)
		{
			entities.insert({entity_id, EntityState{Values(num_values), Values(num_values), Values(num_values)}});
		}
		else
		{
			// Entity is deleted.
			entities.erase(entity_id);
		}
	}

	// Decode diff for each entity.
	// entities must iterate in order.
	Values entity_diff;
	entity_diff.resize(num_values);
	for (auto& kvp : entities)
	{
		auto& entity_state = kvp.second;

		auto common_value_diff_index = common_value_diff_index_encoder.DecodeSymbol(bit_unpacker, false);
		if (common_value_diff_index == 0)
		{
			// This entity had a diff not in the dictionary, decode explicit diff.
			assert(entity_state.current_entity_values.size() == num_values);
			for (auto v = 0; v < num_values; v++)
			{
				entity_diff[v] = value_encoder.DecodeUint64Varint(bit_unpacker, false);
			}
		}
		else
		{
			// Find the last diff entries.
			auto last_entity_diff_it = common_value_diff_indices.find(entity_state.last_entity_diff);
			if (last_entity_diff_it != common_value_diff_indices.end())
			{
				// This entity has an entry for it's last diff.
				auto& last_next_diff_indices_list = common_value_diffs[last_entity_diff_it->second].next_diff_indices;
				if (common_value_diff_index - 1 < last_next_diff_indices_list.size())
				{
					// The encoded index is in the next diff list.
					auto value_diff_index = last_next_diff_indices_list[common_value_diff_index - 1];
					assert(value_diff_index < common_value_diffs.size());

					entity_diff = common_value_diffs[value_diff_index].diff;
				}
				else
				{
					// The encoded index is in the global list.
					assert(common_value_diff_index - 1 - last_next_diff_indices_list.size() < common_value_diffs.size());
					entity_diff = common_value_diffs[common_value_diff_index - 1 - last_next_diff_indices_list.size()].diff;
				}
			}
			else
			{
				// The encoded index is just the global list.
				assert(common_value_diff_index - 1 < common_value_diffs.size());
				entity_diff = common_value_diffs[common_value_diff_index - 1].diff;
			}
		}

		for (auto v = 0; v < num_values; v++)
		{
			auto zigzag_diff = entity_diff[v];
			auto diff = ZigZagDecode(zigzag_diff);
			entity_state.current_entity_values[v] += diff;
		}

		entity_state.last_entity_diff = entity_diff;
	}
}

void BackgroundEncoder::UpdateEntity(EntityId entity_id, const Values& entity_values)
{
	auto entity_it = entities.find(entity_id);
	if (entity_it == entities.end())
	{
		AddEntity(entity_id, entity_values);
		return;
	}

	auto& entity_state = entity_it->second;
	assert(entity_values.size() == entity_state.current_entity_values.size());
	entity_state.current_entity_values = entity_values;
}

// After an entity is deleted it cannot be added again.
void BackgroundEncoder::DeleteEntity(EntityId entity_id)
{
	entities_deleted_since_flush.insert(entity_id);
}

BackgroundEncoder::TrainingData BackgroundEncoder::GetTrainingData() const
{
	std::vector<std::pair<std::uint64_t, CommonValueDiff>> common_value_diffs_vector;
	for (auto& kvp : value_frequencies)
	{
		CommonValueDiff common_value_diff{kvp.first, {}};

		auto next_diff_frequency_it = next_diff_frequencies.find(kvp.first);
		if (next_diff_frequency_it != next_diff_frequencies.end())
		{
			std::vector<std::pair<std::uint64_t, Values>> sorted_frequencies;
		}
		common_value_diffs_vector.push_back({kvp.second, CommonValueDiff{kvp.first, {}}});
	}

	std::sort(
		common_value_diffs_vector.begin(), common_value_diffs_vector.end(), [](auto& a, auto& b) { return b.first < a.first; });

	TrainingData training_data{};
	training_data.entity_number_frequencies = entity_number_encoder.GetSymbolFrequencies();
	training_data.entity_id_diff_frequencies = entity_id_diff_encoder.GetSymbolFrequencies();
	training_data.common_value_diff_index_frequencies = common_value_diff_index_encoder.GetSymbolFrequencies();
	training_data.value_frequencies = value_encoder.GetSymbolFrequencies();

	std::map<Values, std::size_t> this_value_to_index_map;
	for (auto i = 0; i < common_value_diffs_vector.size() && i < num_dictionary_elements; i++)
	{
		training_data.common_value_diffs.push_back(common_value_diffs_vector[i].second);
		this_value_to_index_map[common_value_diffs_vector[i].second.diff] = i;
	}

	// Find next_diffs
	for (auto& common_value_diff : training_data.common_value_diffs)
	{
		auto next_diff_frequency_it = next_diff_frequencies.find(common_value_diff.diff);
		if (next_diff_frequency_it != next_diff_frequencies.end())
		{
			std::vector<std::pair<std::uint64_t, Values>> sorted_frequencies;
			std::vector<std::pair<std::uint64_t, std::uint64_t>> freqs;
			for (auto& kvp : next_diff_frequency_it->second)
			{
				sorted_frequencies.push_back({kvp.second, kvp.first});

				freqs.push_back({kvp.second, kvp.second});
			}
			std::sort(sorted_frequencies.begin(), sorted_frequencies.end(), [](auto& a, auto& b) { return b.first < a.first; });

			for (auto i = 0; i < sorted_frequencies.size() && i < num_next_diff_elements; i++)
			{
				auto value_index_it = this_value_to_index_map.find(sorted_frequencies[i].second);
				if (value_index_it != this_value_to_index_map.end())
				{
					common_value_diff.next_diff_indices.push_back(value_index_it->second);
				}
			}
		}
	}

	return training_data;
}

void BackgroundEncoder::LoadTrainingData(TrainingData training_data)
{
	entity_number_encoder = {training_data.entity_number_frequencies.size(), training_data.entity_number_frequencies};
	entity_id_diff_encoder = {training_data.entity_id_diff_frequencies.size(), training_data.entity_id_diff_frequencies};
	common_value_diff_index_encoder = {
		training_data.common_value_diff_index_frequencies.size(), training_data.common_value_diff_index_frequencies};
	value_encoder = {training_data.value_frequencies.size(), training_data.value_frequencies};

	common_value_diffs = std::move(training_data.common_value_diffs);
	common_value_diff_indices.clear();
	for (auto i = 0; i < common_value_diffs.size(); i++)
	{
		common_value_diff_indices[common_value_diffs[i].diff] = i;
	}
	std::cout << "Loaded " << common_value_diffs.size() << " common value diffs." << std::endl;
}

void BackgroundEncoder::AddEntity(EntityId entity_id, Values entity_values)
{
	assert(entity_values.size() == num_values);
	entities.insert({entity_id, EntityState{Values(num_values), Values(num_values), entity_values}});
}

std::uint64_t BackgroundEncoder::ZigZagEncode(std::int64_t n)
{
	return (n << 1) ^ (n >> 63);
}

std::int64_t BackgroundEncoder::ZigZagDecode(std::uint64_t n)
{
	return static_cast<std::int64_t>(n >> 1ull) ^ -(static_cast<std::int64_t>(n) & 1ll);
}
