// Copyright JAA Contributors 2024-2025

#include "Modules/LocalFetchModule.h"

/**
 * A list of classes allowed to be used by Local Fetch
 * Defined in CPP
 */
TArray<FString> LocalFetchAcceptedTypes = {
	"Texture2D",
	// "TextureCube",
	// "VolumeTexture",
	"TextureRenderTarget2D",

	"", // separator

	"Material",
	"MaterialInterface",
	"MaterialFunction",
	"MaterialInstanceConstant",
	"MaterialParameterCollection",
	"NiagaraParameterCollection",

	"", // separator

	"CurveFloat",
	"CurveTable",
	"CurveVector",
	"CurveLinearColorAtlas",
	"CurveLinearColor",

	"", // separator

	"SoundWave",
	"SoundCue",
	"ReverbEffect",
	"SoundAttenuation",
	"SoundConcurrency",
	"SoundClass",
	"SoundMix",
	"SoundModulationPatch",
		
	"", // separator

	"PhysicalMaterial",
	"SubsurfaceProfile",
	"LandscapeGrassType",
	"DataTable",
};