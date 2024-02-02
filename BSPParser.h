#pragma once

#include <cstdint>
#include <stdexcept>
#include <span>

#include "FileFormat/Parser.h"

struct BSPTexture
{
	BSPEnums::SURF flags = BSPEnums::SURF::NONE;
	BSPStructs::Vector reflectivity{0, 0, 0};
	const char* path = nullptr;
	int32_t width = 0, height = 0;
};

struct BSPStaticProp
{
	BSPStructs::Vector pos{};
	BSPStructs::QAngle ang{};
	const char* model = nullptr;
	int32_t skin = 0;
};

class BSPMap
{
private:
	bool mIsValid = false;

	// Raw data
	uint8_t* mpData;
	size_t mDataSize = 0U;

	// Raw BSP structs
	const BSPStructs::Header* mpHeader;

	const BSPStructs::GameLump* mpGameLumps;
	size_t mNumGameLumps = 0U;

	const BSPStructs::Vector* mpVertices;
	size_t mNumVertices = 0U;

	const BSPStructs::Plane* mpPlanes;
	size_t mNumPlanes = 0U;

	const BSPStructs::Edge* mpEdges;
	size_t mNumEdges = 0U;

	const int32_t* mpSurfEdges;
	size_t mNumSurfEdges = 0U;

	const BSPStructs::Face* mpFaces;
	size_t mNumFaces = 0U;

	const BSPStructs::TexInfo* mpTexInfos;
	size_t mNumTexInfos = 0U;

	const BSPStructs::TexData* mpTexDatas;
	size_t mNumTexDatas = 0U;

	const int32_t* mpTexDataStringTable;
	size_t mNumTexDataStringTableEntries = 0U;

	const char* mpTexDataStringData;
	size_t mNumTexDataStringDatas = 0U;

	const BSPStructs::Model* mpModels;
	size_t mNumModels = 0U;

	const BSPStructs::DispInfo* mpDispInfos;
	size_t mNumDispInfos = 0U;

	const BSPStructs::DispVert* mpDispVerts;
	size_t mNumDispVerts = 0U;

	const BSPStructs::DetailObjectDict* mpDetailObjectDict;
	size_t mNumDetailObjectDictEntries = 0U;

	const BSPStructs::DetailObject* mpDetailObjects;
	size_t mNumDetailObjects = 0U;

	const BSPStructs::StaticPropDict* mpStaticPropDict;
	size_t mNumStaticPropDictEntries = 0U;

	const BSPStructs::StaticPropLeaf* mpStaticPropLeaves;
	size_t mNumStaticPropLeaves = 0U;

	uint16_t mStaticPropsVersion;
	const BSPStructs::StaticPropV4* mpStaticPropsV4;
	const BSPStructs::StaticPropV5* mpStaticPropsV5;
	const BSPStructs::StaticPropV6* mpStaticPropsV6;
	int32_t mNumStaticProps = 0U;

	// Triangulation

	// Whether to create CW tris or CCW tris
	bool mClockwise = true;

	size_t mNumTris = 0U;

	BSPStructs::Vector* mpPositions = nullptr;
	BSPStructs::Vector* mpNormals = nullptr, * mpTangents = nullptr, * mpBinormals = nullptr;
	float* mpUVs = nullptr, * mpAlphas = nullptr;
	int16_t* mpTexIndices = nullptr;

	template<class LumpDatatype>
	bool ParseLump(const LumpDatatype** pArray, size_t* pLength)
	{
		return BSPParser::ParseLump(
			mpData, mDataSize,
			mpHeader,
			pArray, pLength
		);
	}

	bool ParseGameLumps();

	template<class StaticProp>
	bool ParseStaticPropLump(const BSPStructs::GameLump& gameLump, const StaticProp** pPtrOut)
	{
		// Game lump under or overflows the file
		if (
			gameLump.offset < 0 ||
			gameLump.offset + gameLump.length > mDataSize
		) return false;

		size_t totalLumpSize = sizeof(int32_t) * 3;
		if (totalLumpSize > gameLump.length) return false; // Game lump size is corrupted

		// Get ptr to number of static prop dict entries
		// no idea why valve decided to put the counts for all of these before each segment of the sprop lump,
		// cause it's a pain to parse
		const uint8_t* pHeader = mpData + gameLump.offset;
		totalLumpSize += *reinterpret_cast<const int32_t*>(pHeader) * sizeof(BSPStructs::StaticPropDict);
		if (
			*reinterpret_cast<const int32_t*>(pHeader) < 0 ||
			totalLumpSize > gameLump.length
		) return false;

		// Save valid number of dict entries
		mNumStaticPropDictEntries = *reinterpret_cast<const int32_t*>(pHeader);

		// Move ptr over the int and save offset ptr
		pHeader += sizeof(int32_t);
		mpStaticPropDict = reinterpret_cast<const BSPStructs::StaticPropDict*>(pHeader);

		// Move ptr over the static prop dictionary entries, and increase lump size by the size of the leaf lump
		// Note that we already set the total lump size to all int32_t counts that should be in the static prop lump
		pHeader += mNumStaticPropDictEntries * sizeof(BSPStructs::StaticPropDict);
		totalLumpSize += *reinterpret_cast<const int32_t*>(pHeader) * sizeof(BSPStructs::StaticPropLeaf);
		if (
			*reinterpret_cast<const int32_t*>(pHeader) < 0 ||
			totalLumpSize > gameLump.length
		) return false;

		// Save num leaves
		mNumStaticPropLeaves = *reinterpret_cast<const int32_t*>(pHeader);

		// Move ptr and save
		pHeader += sizeof(int32_t);
		mpStaticPropLeaves = reinterpret_cast<const BSPStructs::StaticPropLeaf*>(pHeader);

		// Move ptr over static prop leaves and add static prop lump size
		pHeader += mNumStaticPropLeaves * sizeof(BSPStructs::StaticPropLeaf);
		totalLumpSize += *reinterpret_cast<const int32_t*>(pHeader) * sizeof(StaticProp);
		if (
			*reinterpret_cast<const int32_t*>(pHeader) < 0 ||
			totalLumpSize != gameLump.length // Use != here as we want to make sure we've read exactly the amount of data the lump was meant to have
		) return false;

		mNumStaticProps = *reinterpret_cast<const int32_t*>(pHeader);

		pHeader += sizeof(int32_t);
		*pPtrOut = reinterpret_cast<const StaticProp*>(pHeader);

		return true;
	}

	template<class StaticProp>
	BSPStaticProp GetStaticPropInternal(const int32_t index, const StaticProp* pStaticProps) const
	{
		if (index < 0 || index >= mNumStaticProps)
			throw std::out_of_range("Static prop index out of bounds");

		uint16_t dictIdx = pStaticProps[index].propType;
		if (dictIdx >= mNumStaticPropDictEntries)
			throw std::out_of_range("Static prop dictionary index out of bounds");

		BSPStaticProp prop;
		prop.pos = pStaticProps[index].origin;
		prop.ang = pStaticProps[index].angles;
		prop.model = mpStaticPropDict[dictIdx].modelName;
		prop.skin = pStaticProps[index].skin;

		return prop;
	}

	bool IsFaceNodraw(const BSPStructs::Face* pFace) const;

	void FreeAll();

	bool CalcUVs(
		int16_t texInfoIdx,
		const BSPStructs::Vector* pos,
		float* pUVs
	) const;

	bool GetSurfEdgeVerts(int32_t index, BSPStructs::Vector* pVertA, BSPStructs::Vector* pVertB = nullptr) const;

	bool Triangulate();

public:
	/**
	 * \brief Parses and triangulates a BSP from raw data
	 * \param pFileData Pointer to the loaded BSP file in memory
	 * \param dataSize Size of the loaded BSP file in memory
	 * \param clockwise Whether to create CW tris or CCW tris
	 * \throw std::runtime_error A runtime exception containing a friendly user-facing message about what specifically went wrong during instantation.
	 */
	BSPMap(const uint8_t* pFileData, size_t dataSize, bool clockwise = true);
	~BSPMap();

	/**
	 * \brief Checks if the BSP was parsed successfully
	 * \return Whether the BSP was parsed successfully
	 */
	[[nodiscard]] bool IsValid() const;

	/**
	 * \return The number of textures loaded in the BSP
	 */
	[[nodiscard]] size_t GetNumTextures() const;

	/**
	 * \param index Index of the texture to get
	 * \return The BSPTexture associated with the given index
	 */
	[[nodiscard]] BSPTexture GetTexture(int16_t index) const;

	/**
	 * \return The number of triangles in the triangulated BSP data
	 */
	[[nodiscard]] size_t GetNumTris() const;

	/**
	 * \return The number of vertices in the triangulated BSP data
	 */
	[[nodiscard]] size_t GetNumVertices() const;

	/**
	 * \return Returns a pointer to the list of vertices in the BSP (castable to floats)
	 */
	[[nodiscard]] const BSPStructs::Vector* GetVertices() const;

	template<typename C, typename CPtr = std::add_const_t<std::add_pointer_t<C>>>
	[[nodiscard]] std::span<C> GetVertices() const
	{
		static_assert(std::is_standard_layout_v<C>, "C must be a standard layout type");
		return std::span<C>{reinterpret_cast<CPtr>(GetVertices()), GetNumVertices()};
	}

	/**
	 * \return Returns a pointer to the list of normals in the BSP (castable to floats)
	 */
    [[nodiscard]] const BSPStructs::Vector* GetNormals() const;

	template<typename C, typename CPtr = std::add_const_t<std::add_pointer_t<C>>>
	[[nodiscard]] std::span<C> GetNormals() const
	{
		static_assert(std::is_standard_layout_v<C>, "C must be a standard layout type");
		return std::span<C>{reinterpret_cast<CPtr>(GetNormals()), GetNumVertices()};
	}

	/**
	 * \return Returns a pointer to the list of tangents in the BSP (castable to floats)
	 */
    [[nodiscard]] const BSPStructs::Vector* GetTangents() const;

	/**
	 * \return Returns a pointer to the list of binormals in the BSP (castable to floats)
	 */
    [[nodiscard]] const BSPStructs::Vector* GetBinormals() const;

	/**
	 * \return Returns a pointer to the list of UVs in the BSP (these are 2-component vectors)
	 */
    [[nodiscard]] const float* GetUVs() const;

	/**
	 * \return Returns a pointer to the list of alpha values in the BSP
	 */
    [[nodiscard]] const float* GetAlphas() const;

	/**
	 * \return Returns a pointer to a list of 16-bit integer indices used to index into the TexInfo lump.
	 */
	[[nodiscard]] const int16_t* GetTriTextures() const;

	/**
	 * \return Returns the number of static props in the BSP
	 */
	[[nodiscard]] int32_t GetNumStaticProps() const;

	/**
	 * \param index The index of the static prop requested
	 * \return Returns a BSPStaticProp associated with the given index
	 */
    [[nodiscard]] BSPStaticProp GetStaticProp(int32_t index) const;
};
