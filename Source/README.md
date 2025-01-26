# Key information for contributors: 🎓

Local Fetch's API is located at [JsonAsAsset/LocalFetch](https://github.com/JsonAsAsset/LocalFetch)
It uses CUE4Parse to read game files, and it interacts with the config files inside of the project.

##### Adding Asset Types
> *Asset types without manual code will use **basic** importing, meaning it will only take the properties of the base object and import them.*
- Normal Asset types are found in [`JsonAsAsset/Private/Importers/Constructor/Importer.cpp`](https://github.com/JsonAsAsset/JsonAsAsset/blob/main/Source/JsonAsAsset/Private/Importers/Constructor/Importer.cpp#L51)
- Adding support for Local Fetch of asset types [`JsonAsAsset/Private/Importers/Utilities/AssetUtilities.cpp`](https://github.com/JsonAsAsset/JsonAsAsset/blob/main/Source/JsonAsAsset/Private/Utilities/AssetUtilities.cpp#L28)

##### Custom Logic for Asset Types

Adding **manual** asset type imports is done in the [`JsonAsAsset/Public/Importers/Types`](https://github.com/JsonAsAsset/JsonAsAsset/tree/main/Source/JsonAsAsset/Public/Importers/Types) folder.

##### Settings

JsonAsAsset's settings are in [`Private/Settings/JsonAsAssetSettings.h`](https://github.com/JsonAsAsset/JsonAsAsset/tree/main/Source/JsonAsAsset/Private/Settings/JsonAsAssetSettings.h)
