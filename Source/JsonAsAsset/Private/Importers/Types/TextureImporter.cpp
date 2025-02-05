// Copyright JAA Contributors 2024-2025

#include "Importers/Types/TextureImporter.h"

#include "detex.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureCube.h"
#include "Engine/VolumeTexture.h"
#include "Factories/TextureRenderTargetFactoryNew.h"
#include "nvimage/DirectDrawSurface.h"
#include "nvimage/Image.h"
#include "Utilities/MathUtilities.h"
#include "Utilities/TextureDecode/TextureNVTT.h"

bool ITextureImporter::ImportTexture2D(UTexture*& OutTexture2D, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	const TSharedPtr<FJsonObject> SubObjectProperties = Properties->GetObjectField(TEXT("Properties"));

	// NEW: .bin support
	UTexture2D* Texture2D = NewObject<UTexture2D>(OutermostPkg, UTexture2D::StaticClass(), *FileName, RF_Standalone | RF_Public);

#if ENGINE_MAJOR_VERSION >= 5
	Texture2D->SetPlatformData(new FTexturePlatformData());
#else
	Texture2D->PlatformData = new FTexturePlatformData();
#endif

	ImportTexture2D_Data(Texture2D, SubObjectProperties);

#if ENGINE_MAJOR_VERSION >= 5
	FTexturePlatformData* PlatformData = Texture2D->GetPlatformData();
#else
	FTexturePlatformData* PlatformData = Texture2D->PlatformData;
#endif

	const int SizeX = Properties->GetNumberField(TEXT("SizeX"));
	const int SizeY = Properties->GetNumberField(TEXT("SizeY"));
	constexpr int SizeZ = 1; // Tex2D doesn't have depth

	const TArray<TSharedPtr<FJsonValue>>* TextureMipsPtr;
	Properties->TryGetArrayField(TEXT("Mips"), TextureMipsPtr);
	if (TextureMipsPtr)
	{
		auto TextureMips = *TextureMipsPtr;
		if (TextureMips.Num() == 1)
			Texture2D->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	}

	FString PixelFormat;
	if (Properties->TryGetStringField(TEXT("PixelFormat"), PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(Texture2D->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	int Size = SizeX * SizeY * (PlatformData->PixelFormat == PF_BC6H ? 16 : 4);
	if (PlatformData->PixelFormat == PF_B8G8R8A8 || PlatformData->PixelFormat == PF_FloatRGBA || PlatformData->PixelFormat == PF_G16) Size = Data.Num();
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));

	GetDecompressedTextureData(Data.GetData(), DecompressedData, SizeX, SizeY, SizeZ, Size, PlatformData->PixelFormat);

	ETextureSourceFormat Format = TSF_BGRA8;
	if (Texture2D->CompressionSettings == TC_HDR) Format = TSF_RGBA16F;
	if (PlatformData->PixelFormat == PF_G16) Format = TSF_G16;
	Texture2D->Source.Init(SizeX, SizeY, 1, 1, Format);
	uint8_t* Dest = Texture2D->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	Texture2D->Source.UnlockMip(0);

	Texture2D->UpdateResource();

	if (Texture2D && Texture2D->IsValidLowLevel() && Texture2D != nullptr)
		{
		OutTexture2D = Texture2D;
		return true;
	}

	return false;
}

bool ITextureImporter::ImportTextureCube(UTexture*& OutTextureCube, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureCube* TextureCube = NewObject<UTextureCube>(Package, UTextureCube::StaticClass(), *FileName, RF_Public | RF_Standalone);

#if ENGINE_MAJOR_VERSION >= 5
	TextureCube->SetPlatformData(new FTexturePlatformData());
#else
	TextureCube->PlatformData = new FTexturePlatformData();
#endif

	ImportTexture_Data(TextureCube, Properties);

#if ENGINE_MAJOR_VERSION >= 5
	FTexturePlatformData* PlatformData = TextureCube->GetPlatformData();
#else
	FTexturePlatformData* PlatformData = TextureCube->PlatformData;
#endif

	const int SizeX = Properties->GetNumberField(TEXT("SizeX"));
	const int SizeY = Properties->GetNumberField(TEXT("SizeY")) / 6;

	FString PixelFormat;
	if (Properties->TryGetStringField(TEXT("PixelFormat"), PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(TextureCube->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	int Size = SizeX * SizeY * (PlatformData->PixelFormat == PF_BC6H ? 16 : 4);
	if (PlatformData->PixelFormat == PF_FloatRGBA) Size = Data.Num();
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));

	ETextureSourceFormat Format = TSF_BGRA8;
	if (TextureCube->CompressionSettings == TC_HDR) Format = TSF_RGBA16F;
	TextureCube->Source.Init(SizeX, SizeY, 1, 1, Format);
	uint8_t* Dest = TextureCube->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	TextureCube->Source.UnlockMip(0);

	TextureCube->PostEditChange();

	if (TextureCube) {
		OutTextureCube = TextureCube;
		return true;
	}

	return false;
}

bool ITextureImporter::ImportVolumeTexture(UTexture*& OutVolumeTexture, TArray<uint8>& Data, const TSharedPtr<FJsonObject>& Properties) const {
	UVolumeTexture* VolumeTexture = NewObject<UVolumeTexture>(Package, UVolumeTexture::StaticClass(), *FileName, RF_Public | RF_Standalone);

#if ENGINE_MAJOR_VERSION >= 5
	VolumeTexture->SetPlatformData(new FTexturePlatformData());
#endif
	FString PixelFormat;

#if ENGINE_MAJOR_VERSION >= 5
	FTexturePlatformData* PlatformData = VolumeTexture->GetPlatformData();
#else
	FTexturePlatformData* PlatformData = VolumeTexture->PlatformData;
#endif
	
	if (Properties->TryGetStringField(TEXT("PixelFormat"), PixelFormat))
		PlatformData->PixelFormat = static_cast<EPixelFormat>(VolumeTexture->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	ImportTexture_Data(VolumeTexture, Properties);

	const int SizeX = Properties->GetNumberField(TEXT("SizeX"));
	const int SizeY = Properties->GetNumberField(TEXT("SizeY"));
	// const int SizeZ = Properties->GetNumberField(TEXT("SizeZ")); // Need to add the property
	const int SizeZ = 1;
	int Size = SizeX * SizeY * SizeZ;

	// Decompression
	uint8* DecompressedData = static_cast<uint8*>(FMemory::Malloc(Size));
	GetDecompressedTextureData(Data.GetData(), DecompressedData, SizeX, SizeY, SizeZ, Size, PlatformData->PixelFormat);

	VolumeTexture->Source.Init(SizeX, SizeY, SizeZ, 1, TSF_BGRA8);

	uint8_t* Dest = VolumeTexture->Source.LockMip(0);
	FMemory::Memcpy(Dest, DecompressedData, Size);
	VolumeTexture->Source.UnlockMip(0);
	VolumeTexture->UpdateResource();

	if (VolumeTexture) {
		OutVolumeTexture = VolumeTexture;
		return true;
	}

	return false;
}

bool ITextureImporter::ImportRenderTarget2D(UTexture*& OutRenderTarget2D, const TSharedPtr<FJsonObject>& Properties) const {
	UTextureRenderTargetFactoryNew* TextureFactory = NewObject<UTextureRenderTargetFactoryNew>();
	TextureFactory->AddToRoot();
	UTextureRenderTarget2D* RenderTarget2D = Cast<UTextureRenderTarget2D>(TextureFactory->FactoryCreateNew(UTextureRenderTarget2D::StaticClass(), OutermostPkg, *FileName, RF_Standalone | RF_Public, nullptr, GWarn));

	ImportTexture_Data(RenderTarget2D, Properties);

	int SizeX;
	if (Properties->TryGetNumberField(TEXT("SizeX"), SizeX)) RenderTarget2D->SizeX = SizeX;
	int SizeY;
	if (Properties->TryGetNumberField(TEXT("SizeY"), SizeY)) RenderTarget2D->SizeY = SizeY;

	FString AddressX;
	if (Properties->TryGetStringField(TEXT("AddressX"), AddressX)) RenderTarget2D->AddressX = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressX));
	FString AddressY;
	if (Properties->TryGetStringField(TEXT("AddressY"), AddressY)) RenderTarget2D->AddressY = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressY));
	FString RenderTargetFormat;
	if (Properties->TryGetStringField(TEXT("RenderTargetFormat"), RenderTargetFormat)) RenderTarget2D->RenderTargetFormat = static_cast<ETextureRenderTargetFormat>(StaticEnum<ETextureRenderTargetFormat>()->GetValueByNameString(RenderTargetFormat));

	bool bAutoGenerateMips;
	if (Properties->TryGetBoolField(TEXT("bAutoGenerateMips"), bAutoGenerateMips)) RenderTarget2D->bAutoGenerateMips = bAutoGenerateMips;
	if (bAutoGenerateMips) {
		FString MipsSamplerFilter;
		
		if (Properties->TryGetStringField(TEXT("MipsSamplerFilter"), MipsSamplerFilter))
			RenderTarget2D->MipsSamplerFilter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(MipsSamplerFilter));
	}

	const TSharedPtr<FJsonObject>* ClearColor;
	if (Properties->TryGetObjectField(TEXT("ClearColor"), ClearColor)) RenderTarget2D->ClearColor = FMathUtilities::ObjectToLinearColor(ClearColor->Get());

	if (RenderTarget2D) {
		OutRenderTarget2D = RenderTarget2D;
		return true;
	}

	return false;
}

// Handle UTexture2D
bool ITextureImporter::ImportTexture2D_Data(UTexture2D* InTexture2D, const TSharedPtr<FJsonObject>& Properties) const {
	if (InTexture2D == nullptr) return false;

	ImportTexture_Data(InTexture2D, Properties);

	FString AddressX;
	FString AddressY;
	bool bHasBeenPaintedInEditor;

	if (Properties->TryGetStringField(TEXT("AddressX"), AddressX)) InTexture2D->AddressX = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressX));
	if (Properties->TryGetStringField(TEXT("AddressY"), AddressY)) InTexture2D->AddressY = static_cast<TextureAddress>(StaticEnum<TextureAddress>()->GetValueByNameString(AddressY));
	if (Properties->TryGetBoolField(TEXT("bHasBeenPaintedInEditor"), bHasBeenPaintedInEditor)) InTexture2D->bHasBeenPaintedInEditor = bHasBeenPaintedInEditor;

	// --------- Platform Data --------- //
#if ENGINE_MAJOR_VERSION >= 5
	FTexturePlatformData* PlatformData = InTexture2D->GetPlatformData();
#else
	FTexturePlatformData* PlatformData = InTexture2D->PlatformData;
#endif
	int SizeX;
	int SizeY;
	uint32 PackedData;
	FString PixelFormat;

	if (Properties->TryGetNumberField(TEXT("SizeX"), SizeX)) PlatformData->SizeX = SizeX;
	if (Properties->TryGetNumberField(TEXT("SizeY"), SizeY)) PlatformData->SizeY = SizeY;
	if (Properties->TryGetNumberField(TEXT("PackedData"), PackedData)) PlatformData->PackedData = PackedData;
	if (Properties->TryGetStringField(TEXT("PixelFormat"), PixelFormat)) PlatformData->PixelFormat = static_cast<EPixelFormat>(InTexture2D->GetPixelFormatEnum()->GetValueByNameString(PixelFormat));

	int FirstResourceMemMip;
	int LevelIndex;
	
	if (Properties->TryGetNumberField(TEXT("FirstResourceMemMip"), FirstResourceMemMip)) InTexture2D->FirstResourceMemMip = FirstResourceMemMip;
	if (Properties->TryGetNumberField(TEXT("LevelIndex"), LevelIndex)) InTexture2D->LevelIndex = LevelIndex;

	return false;
}

// Handle UTexture
bool ITextureImporter::ImportTexture_Data(UTexture* InTexture, const TSharedPtr<FJsonObject>& Properties) const {
	if (InTexture == nullptr) return false;

	// Declare variables before the if statements
#if ENGINE_MAJOR_VERSION >= 5
	float AdjustBrightness = 0.0f;
	float AdjustBrightnessCurve = 0.0f;
	float AdjustHue = 0.0f;
	float AdjustMaxAlpha = 0.0f;
	float AdjustMinAlpha = 0.0f;
	float AdjustRGBCurve = 0.0f;
	float AdjustSaturation = 0.0f;
	float AdjustVibrance = 0.0f;
	float ChromaKeyThreshold = 0.0f;
	float CompositePower = 0.0f;
#else
	double AdjustBrightness = 0.0f;
	double AdjustBrightnessCurve = 0.0f;
	double AdjustHue = 0.0f;
	double AdjustMaxAlpha = 0.0f;
	double AdjustMinAlpha = 0.0f;
	double AdjustRGBCurve = 0.0f;
	double AdjustSaturation = 0.0f;
	double AdjustVibrance = 0.0f;
	double ChromaKeyThreshold = 0.0f;
	double CompositePower = 0.0f;
#endif

	if (Properties->TryGetNumberField(TEXT("AdjustBrightness"), AdjustBrightness))
	    InTexture->AdjustBrightness = AdjustBrightness;

	if (Properties->TryGetNumberField(TEXT("AdjustBrightnessCurve"), AdjustBrightnessCurve))
	    InTexture->AdjustBrightnessCurve = AdjustBrightnessCurve;

	if (Properties->TryGetNumberField(TEXT("AdjustHue"), AdjustHue))
	    InTexture->AdjustHue = AdjustHue;

	if (Properties->TryGetNumberField(TEXT("AdjustMaxAlpha"), AdjustMaxAlpha))
	    InTexture->AdjustMaxAlpha = AdjustMaxAlpha;

	if (Properties->TryGetNumberField(TEXT("AdjustMinAlpha"), AdjustMinAlpha))
	    InTexture->AdjustMinAlpha = AdjustMinAlpha;

	if (Properties->TryGetNumberField(TEXT("AdjustRGBCurve"), AdjustRGBCurve))
	    InTexture->AdjustRGBCurve = AdjustRGBCurve;

	if (Properties->TryGetNumberField(TEXT("AdjustSaturation"), AdjustSaturation))
	    InTexture->AdjustSaturation = AdjustSaturation;

	if (Properties->TryGetNumberField(TEXT("AdjustVibrance"), AdjustVibrance))
	    InTexture->AdjustVibrance = AdjustVibrance;

	const TSharedPtr<FJsonObject>* AlphaCoverageThresholds = nullptr;
	if (Properties->TryGetObjectField(TEXT("AlphaCoverageThresholds"), AlphaCoverageThresholds))
	    InTexture->AlphaCoverageThresholds = FMathUtilities::ObjectToVector(AlphaCoverageThresholds->Get());

	bool bChromaKeyTexture = false;
	if (Properties->TryGetBoolField(TEXT("bChromaKeyTexture"), bChromaKeyTexture))
	    InTexture->bChromaKeyTexture = bChromaKeyTexture;

	bool bFlipGreenChannel = false;
	if (Properties->TryGetBoolField(TEXT("bFlipGreenChannel"), bFlipGreenChannel))
	    InTexture->bFlipGreenChannel = bFlipGreenChannel;

	bool bNoTiling = false;
	if (Properties->TryGetBoolField(TEXT("bNoTiling"), bNoTiling))
	    InTexture->bNoTiling = bNoTiling;

	bool bPreserveBorder = false;
	if (Properties->TryGetBoolField(TEXT("bPreserveBorder"), bPreserveBorder))
	    InTexture->bPreserveBorder = bPreserveBorder;

	bool bUseLegacyGamma = false;
	if (Properties->TryGetBoolField(TEXT("bUseLegacyGamma"), bUseLegacyGamma))
	    InTexture->bUseLegacyGamma = bUseLegacyGamma;

	const TSharedPtr<FJsonObject>* ChromaKeyColor = nullptr;
	if (Properties->TryGetObjectField(TEXT("ChromaKeyColor"), ChromaKeyColor))
	    InTexture->ChromaKeyColor = FMathUtilities::ObjectToColor(ChromaKeyColor->Get());

	if (Properties->TryGetNumberField(TEXT("ChromaKeyThreshold"), ChromaKeyThreshold))
	    InTexture->ChromaKeyThreshold = ChromaKeyThreshold;

	if (Properties->TryGetNumberField(TEXT("CompositePower"), CompositePower))
	    InTexture->CompositePower = CompositePower;

	FString CompositeTextureMode = TEXT("");
	if (Properties->TryGetStringField(TEXT("CompositeTextureMode"), CompositeTextureMode))
	    InTexture->CompositeTextureMode = static_cast<ECompositeTextureMode>(StaticEnum<ECompositeTextureMode>()->GetValueByNameString(CompositeTextureMode));

	bool CompressionNoAlpha = false;
	if (Properties->TryGetBoolField(TEXT("CompressionNoAlpha"), CompressionNoAlpha))
	    InTexture->CompressionNoAlpha = CompressionNoAlpha;

	bool CompressionNone = false;
	if (Properties->TryGetBoolField(TEXT("CompressionNone"), CompressionNone))
	    InTexture->CompressionNone = CompressionNone;

	FString CompressionQuality = TEXT("");
	if (Properties->TryGetStringField(TEXT("CompressionQuality"), CompressionQuality))
	    InTexture->CompressionQuality = static_cast<ETextureCompressionQuality>(StaticEnum<ETextureCompressionQuality>()->GetValueByNameString(CompressionQuality));

	FString CompressionSettings = TEXT("");
	if (Properties->TryGetStringField(TEXT("CompressionSettings"), CompressionSettings))
	    InTexture->CompressionSettings = static_cast<TextureCompressionSettings>(StaticEnum<TextureCompressionSettings>()->GetValueByNameString(CompressionSettings));

	bool CompressionYCoCg = false;
	if (Properties->TryGetBoolField(TEXT("CompressionYCoCg"), CompressionYCoCg))
	    InTexture->CompressionYCoCg = CompressionYCoCg;

	bool DeferCompression = false;
	if (Properties->TryGetBoolField(TEXT("DeferCompression"), DeferCompression))
	    InTexture->DeferCompression = DeferCompression;

	FString Filter = TEXT("");
	if (Properties->TryGetStringField(TEXT("Filter"), Filter))
	    InTexture->Filter = static_cast<TextureFilter>(StaticEnum<TextureFilter>()->GetValueByNameString(Filter));

	// TODO: Add LayerFormatSettings
	FString LODGroup;
	FString LossyCompressionAmount;

	if (Properties->TryGetStringField(TEXT("LODGroup"), LODGroup))
		InTexture->LODGroup = static_cast<TextureGroup>(StaticEnum<TextureGroup>()->GetValueByNameString(LODGroup));
	if (Properties->TryGetStringField(TEXT("LossyCompressionAmount"), LossyCompressionAmount))
		InTexture->LossyCompressionAmount = static_cast<ETextureLossyCompressionAmount>(StaticEnum<ETextureLossyCompressionAmount>()->GetValueByNameString(LossyCompressionAmount));

	int MaxTextureSize;
	FString MipGenSettings;
	FString MipLoadOptions;
	
	if (Properties->TryGetNumberField(TEXT("MaxTextureSize"), MaxTextureSize))
		InTexture->MaxTextureSize = MaxTextureSize;
	if (Properties->TryGetStringField(TEXT("MipGenSettings"), MipGenSettings))
		InTexture->MipGenSettings = static_cast<TextureMipGenSettings>(StaticEnum<TextureMipGenSettings>()->GetValueByNameString(MipGenSettings));
	if (Properties->TryGetStringField(TEXT("MipLoadOptions"), MipLoadOptions))
		InTexture->MipLoadOptions = static_cast<ETextureMipLoadOptions>(StaticEnum<ETextureMipLoadOptions>()->GetValueByNameString(MipLoadOptions));

	FString PowerOfTwoMode;
	const TSharedPtr<FJsonObject>* PaddingColor;
	
	if (Properties->TryGetObjectField(TEXT("PaddingColor"), PaddingColor)) InTexture->PaddingColor = FMathUtilities::ObjectToColor(PaddingColor->Get());
	if (Properties->TryGetStringField(TEXT("PowerOfTwoMode"), PowerOfTwoMode))
		InTexture->PowerOfTwoMode = static_cast<ETexturePowerOfTwoSetting::Type>(StaticEnum<ETexturePowerOfTwoSetting::Type>()->GetValueByNameString(PowerOfTwoMode));

	bool SRGB;
	bool VirtualTextureStreaming;
	
	if (Properties->TryGetBoolField(TEXT("SRGB"), SRGB))
		InTexture->SRGB = SRGB;
	if (Properties->TryGetBoolField(TEXT("VirtualTextureStreaming"), VirtualTextureStreaming))
		InTexture->VirtualTextureStreaming = VirtualTextureStreaming;
	
	FString LightingGuid;
	if (Properties->TryGetStringField(TEXT("LightingGuid"), LightingGuid))
		InTexture->SetLightingGuid(FGuid(LightingGuid));

	return false;
}

void ITextureImporter::GetDecompressedTextureData(uint8* Data, uint8*& OutData, const int SizeX, const int SizeY, const int SizeZ, const int TotalSize, const EPixelFormat Format) const {
	// NOTE: Not all formats are supported, feel free to add
	//       if needed. Formats may need other dependencies.
	switch (Format) {
	case PF_BC7: {
		detexTexture Texture;
		Texture.data = Data;
		Texture.format = DETEX_TEXTURE_FORMAT_BPTC;
		Texture.width = SizeX;
		Texture.height = SizeY;
		Texture.width_in_blocks = SizeX / 4;
		Texture.height_in_blocks = SizeY / 4;

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	case PF_BC6H: {
		detexTexture Texture;
		Texture.data = Data;
		Texture.format = DETEX_TEXTURE_FORMAT_BPTC_FLOAT;
		Texture.width = SizeX;
		Texture.height = SizeY;
		Texture.width_in_blocks = SizeX / 4;
		Texture.height_in_blocks = SizeY / 4;

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	case PF_DXT5: {
		detexTexture Texture;
		{
			Texture.data = Data;
			Texture.format = DETEX_TEXTURE_FORMAT_BC3;
			Texture.width = SizeX;
			Texture.height = SizeY;
			Texture.width_in_blocks = SizeX / 4;
			Texture.height_in_blocks = SizeY / 4;
		}

		detexDecompressTextureLinear(&Texture, OutData, DETEX_PIXEL_FORMAT_BGRA8);
	}
	break;

	// Gray/Grey, not Green, typically actually uses a red format with replication of R to RGB
	case PF_G8: {
		const uint8* s = Data;
		uint8* d = OutData;

		for (int i = 0; i < SizeX * SizeY; i++) {
			const uint8 b = *s++;
			*d++ = b;
			*d++ = b;
			*d++ = b;
			*d++ = 255;
		}
	}
	break;

	// FloatRGBA: 16F
	// G16: Gray/Grey like G8
	case PF_B8G8R8A8:
	case PF_FloatRGBA:
	case PF_G16: {
		FMemory::Memcpy(OutData, Data, TotalSize);
	}
	break;

	default: {
		nv::DDSHeader Header;
		nv::Image Image;

		uint FourCC;
		switch (Format) {
		case PF_BC4:
			FourCC = FOURCC_ATI1;
			break;
		case PF_BC5:
			FourCC = FOURCC_ATI2;
			break;
		case PF_DXT1:
			FourCC = FOURCC_DXT1;
			break;
		case PF_DXT3:
			FourCC = FOURCC_DXT3;
			break;
		default: FourCC = 0;
		}

		Header.setFourCC(FourCC);
		Header.setWidth(SizeX);
		Header.setHeight(SizeY);
		Header.setDepth(SizeZ);
		Header.setNormalFlag(Format == PF_BC5);
		DecodeDDS(Data, SizeX, SizeY, SizeZ, Header, Image);

		FMemory::Memcpy(OutData, Image.pixels(), TotalSize);
	}
	break;
	}
}
