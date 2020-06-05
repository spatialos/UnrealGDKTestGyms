// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class NFRConstants
{
	int64 TimeToStartFPSSampling;
	bool bFPSSamplingValid{false};
public:
	float MinServerFPS = 20.0f;
	float MinClientFPS = 20.0f;
	
	NFRConstants();
	bool SamplesForFPSValid(); // Wait until this is true until checking FPS values
	static NFRConstants& Get();
};