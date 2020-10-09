// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#pragma once

#include "BitPacker.h"
#include "BitUnpacker.h"
#include "PrefixEncoding.h"

#include <map>
#include <set>
#include <unordered_map>

// A `BackgroundEncoder` is a view of entity state, where each entity has N
// associated integer values, where N is the same for all entities.
//
// The view is able to be modified, and then flushed into a network delta. If
// all network deltas are applied exactly once, in order, to another
// `BackgroundEncoder`, the view will remain consistent.
//
// `CreateInitialNetworkDelta()` will provide a single network delta which can
// be applied to an empty `BackgroundEncoder` to put the empty
// `BackgroundEncoder` into the same state as the original `BackgroundEncoder`.
// This can be used for when a client logs in for the first time. Subsequent
// flushed network deltas can then be applied to keep the client consistent.
//
// The encoder aims to keep the network deltas as small as possible.
//
// # Training data
// Two `BackgroundEncoder` must use exactly the same `TrainingData` to work. The
// training data should be set before any activity. This training data can be
// serialised with the associated methods.
//
// # How it works
// For each network delta, the encoder aims to send integer diffs of the entity
// values. However, the encoder has a shared dictionary of common sets of diffs
// (for all values together). For example, with an encoder with three values per
// entity, the dictionary may contain (+5, -1, +3) as an item.
//
// For each entity that has changed values, the encoder then writes the
// dictionary index, or the explicit value diffs if the diff is not in the
// dictionary.
//
// In order to effectively send dictionary indices, as well as entity ID diffs
// (for the entity list), the `BackgroundEncoder` uses the `PrefixEncoding`
// class, which is a prefix code encoding that minimises the bits used to send
// frequent symbols. This encoding is also derived from the training data.
//
// Each dictionary element also holds a small list of other dictionary elements
// which often procede the first element's diff. This dictionary index is sent
// instead, which is a smaller integer and can be encoded in fewer bits.
//
// This encodes the assumption that a player moving in some direction at some
// speed will often continue moving in that rough direction and speed.
struct BackgroundEncoder
{
public:
	using EntityId = std::int64_t;
	using ByteArray = std::vector<std::uint8_t>;

	using Values = std::vector<std::uint64_t>;
	struct CommonValueDiff
	{
		Values diff;
		std::vector<std::size_t> next_diff_indices;
	};

	struct TrainingData
	{
		std::vector<std::uint64_t> entity_number_frequencies;
		std::vector<std::uint64_t> entity_id_diff_frequencies;
		std::vector<std::uint64_t> common_value_diff_index_frequencies;
		std::vector<std::uint64_t> value_frequencies;
		std::vector<CommonValueDiff> common_value_diffs;

		ByteArray Serialise() const;

		static TrainingData Deserialise(const ByteArray& serialised);
	};

	BackgroundEncoder(std::size_t num_values, std::size_t num_next_diff_elements, std::size_t num_dictionary_elements);

	std::size_t GetNumValuesPerEntity() const;

	std::size_t GetNumEntities() const;

	std::vector<EntityId> GetEntityIds() const;

	const Values* GetEntityValues(EntityId entity_id) const;

	ByteArray CreateInitialData();

	void ApplyInitialData(const ByteArray& initial_data);

	ByteArray FlushNetworkDelta();

	void ApplyNetworkDelta(const ByteArray& network_delta);

	void UpdateEntity(EntityId entity_id, const Values& entity_values);

	// After an entity is deleted it cannot be added again.
	void DeleteEntity(EntityId entity_id);

	TrainingData GetTrainingData() const;

	void LoadTrainingData(TrainingData training_data);

private:
	const std::size_t num_values;
	const std::size_t num_next_diff_elements;
	const std::size_t num_dictionary_elements;

	struct EntityState
	{
		Values last_entity_values;
		Values last_entity_diff;
		Values current_entity_values;
	};
	std::map<EntityId, EntityState> entities;
	std::map<Values, std::uint64_t> value_frequencies;
	std::map<Values, std::map<Values, std::uint64_t>> next_diff_frequencies;

	std::vector<CommonValueDiff> common_value_diffs;
	std::map<Values, std::size_t> common_value_diff_indices;
	std::set<EntityId> entities_deleted_since_flush;

	std::set<EntityId> last_flushed_entity_id_set;

	PrefixEncoding entity_number_encoder;
	PrefixEncoding entity_id_diff_encoder;
	PrefixEncoding common_value_diff_index_encoder;
	PrefixEncoding value_encoder;

	void AddEntity(EntityId entity_id, Values entity_values);

	static std::uint64_t ZigZagEncode(std::int64_t n);

	static std::int64_t ZigZagDecode(std::uint64_t n);
};