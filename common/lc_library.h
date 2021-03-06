#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

#include "lc_context.h"
#include "lc_mesh.h"
#include "lc_math.h"
#include "lc_array.h"

class PieceInfo;
class lcZipFile;

enum lcZipFileType
{
	LC_ZIPFILE_OFFICIAL,
	LC_ZIPFILE_UNOFFICIAL,
	LC_NUM_ZIPFILES
};

struct lcLibraryMeshVertex
{
	lcVector3 Position;
	lcVector3 Normal;
	float NormalWeight;
};

struct lcLibraryMeshVertexTextured
{
	lcVector3 Position;
	lcVector3 Normal;
	float NormalWeight;
	lcVector2 TexCoord;
};

class lcLibraryMeshSection
{
public:
	lcLibraryMeshSection(lcMeshPrimitiveType PrimitiveType, lcuint32 Color, lcTexture* Texture)
		: mIndices(1024, 1024)
	{
		mPrimitiveType = PrimitiveType;
		mColor = Color;
		mTexture = Texture;
	}

	~lcLibraryMeshSection()
	{
	}

	lcMeshPrimitiveType mPrimitiveType;
	lcuint32 mColor;
	lcTexture* mTexture;
	lcArray<lcuint32> mIndices;
};

struct lcLibraryTextureMap
{
	lcVector4 Params[2];
	lcTexture* Texture;
	bool Fallback;
	bool Next;
};

enum lcMeshDataType
{
	LC_MESHDATA_HIGH,
	LC_MESHDATA_LOW,
	LC_MESHDATA_SHARED,
	LC_NUM_MESHDATA_TYPES
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mVertices[MeshDataIdx].SetGrow(1024);
	}

	~lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mSections[MeshDataIdx].DeleteAll();
	}

	bool IsEmpty() const
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			if (!mSections[MeshDataIdx].IsEmpty())
				return false;

		return true;
	}

	lcLibraryMeshSection* AddSection(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, lcuint32 ColorCode, lcTexture* Texture);
	lcuint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, bool Optimize);
	lcuint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, bool Optimize);
	lcuint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector2& TexCoord, bool Optimize);
	lcuint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize);
	void AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcLibraryMeshVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, lcuint32 ColorCode, int IndexCount, lcuint32** IndexBuffer);
	void AddLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, bool WindingCCW, const lcVector3* Vertices, bool Optimize);
	void AddTexturedLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, bool WindingCCW, const lcLibraryTextureMap& Map, const lcVector3* Vertices, bool Optimize);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void TestQuad(int* QuadIndices, const lcVector3* Vertices);
	void ResequenceQuad(int* QuadIndices, int a, int b, int c, int d);

	lcArray<lcLibraryMeshSection*> mSections[LC_NUM_MESHDATA_TYPES];
	lcArray<lcLibraryMeshVertex> mVertices[LC_NUM_MESHDATA_TYPES];
	lcArray<lcLibraryMeshVertexTextured> mTexturedVertices[LC_NUM_MESHDATA_TYPES];
};

class lcLibraryPrimitive
{
public:
	lcLibraryPrimitive(const char* Name, lcZipFileType ZipFileType,lcuint32 ZipFileIndex, bool Stud, bool SubFile)
	{
		strncpy(mName, Name, sizeof(mName));
		mName[sizeof(mName) - 1] = 0;

		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
		mLoaded = false;
		mStud = Stud;
		mSubFile = SubFile;
	}

	void SetZipFile(lcZipFileType ZipFileType,lcuint32 ZipFileIndex)
	{
		mZipFileType = ZipFileType;
		mZipFileIndex = ZipFileIndex;
	}

	char mName[LC_MAXPATH];
	lcZipFileType mZipFileType;
	lcuint32 mZipFileIndex;
	bool mLoaded;
	bool mStud;
	bool mSubFile;
	lcLibraryMeshData mMeshData;
};

class lcPiecesLibrary : public QObject
{
	Q_OBJECT

public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	bool Load(const char* LibraryPath);
	void Unload();
	void RemoveTemporaryPieces();
	void RemovePiece(PieceInfo* Info);

	PieceInfo* FindPiece(const char* PieceName, Project* Project, bool CreatePlaceholder, bool SearchProjectFolder);
	void LoadPieceInfo(PieceInfo* Info, bool Wait, bool Priority);
	void ReleasePieceInfo(PieceInfo* Info);
	bool LoadBuiltinPieces();
	bool LoadPieceData(PieceInfo* Info);
	void LoadQueuedPiece();
	void WaitForLoadQueue();

	lcTexture* FindTexture(const char* TextureName);
	bool LoadTexture(lcTexture* Texture);

	bool PieceInCategory(PieceInfo* Info, const char* CategoryKeywords) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces);
	void GetPatternedPieces(PieceInfo* Parent, lcArray<PieceInfo*>& Pieces) const;
	void GetParts(lcArray<PieceInfo*>& Parts);

	bool IsPrimitive(const char* Name) const
	{
		return FindPrimitiveIndex(Name) != -1;
	}

	void SetOfficialPieces()
	{
		if (mZipFiles[LC_ZIPFILE_OFFICIAL])
			mNumOfficialPieces = mPieces.GetSize();
	}

	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, bool InvertWinding, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData, lcMeshDataType MeshDataType, bool Optimize);
	lcMesh* CreateMesh(PieceInfo* Info, lcLibraryMeshData& MeshData);
	void ReleaseBuffers(lcContext* Context);
	void UpdateBuffers(lcContext* Context);
	void UnloadUnusedParts();

	lcArray<PieceInfo*> mPieces;
	lcArray<lcLibraryPrimitive*> mPrimitives;
	int mNumOfficialPieces;

	lcArray<lcTexture*> mTextures;

	char mLibraryPath[LC_MAXPATH];

	bool mBuffersDirty;
	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;

signals:
	void PartLoaded(PieceInfo* Info);

protected:
	bool OpenArchive(const char* FileName, lcZipFileType ZipFileType);
	bool OpenArchive(lcFile* File, const char* FileName, lcZipFileType ZipFileType);
	bool OpenDirectory(const char* Path);
	void ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName);

	bool ReadCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool WriteCacheFile(const QString& FileName, lcMemFile& CacheFile);
	bool LoadCacheIndex(const QString& FileName);
	bool SaveCacheIndex(const QString& FileName);
	bool LoadCachePiece(PieceInfo* Info);
	bool SaveCachePiece(PieceInfo* Info);

	int FindPrimitiveIndex(const char* Name) const;
	bool LoadPrimitive(int PrimitiveIndex);

	QMutex mLoadMutex;
	QList<QFuture<void>> mLoadFutures;
	QList<PieceInfo*> mLoadQueue;

	QString mCachePath;
	qint64 mArchiveCheckSum[4];
	char mLibraryFileName[LC_MAXPATH];
	char mUnofficialFileName[LC_MAXPATH];
	lcZipFile* mZipFiles[LC_NUM_ZIPFILES];
	bool mHasUnofficial;
};

#endif // _LC_LIBRARY_H_
