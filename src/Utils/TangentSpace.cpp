#include "TangentSpace.h"
#include "mikktspace.h"

struct MeshData
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;
	std::vector<unsigned int> faces;

	std::vector<glm::vec4> tangents;
};

int getNumFaces(const SMikkTSpaceContext* pContext)
{
	const MeshData* mdata = (const MeshData*)pContext->m_pUserData;
	return mdata->faces.size() / 3;
}

int getNumVerticesOfFace(
	const SMikkTSpaceContext* pContext,
	const int iFace)
{
	return 3;
}

void getPosition(
	const SMikkTSpaceContext* pContext,
	float fvPosOut[],
	const int iFace,
	const int iVert)
{
	const MeshData* mdata = (const MeshData*)pContext->m_pUserData;
	int v = mdata->faces[iFace * 3 + iVert];
	((glm::vec3*)fvPosOut)[0] = mdata->positions[v];
}

void getNormal(
	const SMikkTSpaceContext* pContext,
	float fvNormOut[],
	const int iFace,
	const int iVert)
{
	const MeshData* mdata = (const MeshData*)pContext->m_pUserData;
	int v = mdata->faces[iFace * 3 + iVert];
	((glm::vec3*)fvNormOut)[0] = mdata->normals[v];
}

void getTexCoord(
	const SMikkTSpaceContext* pContext,
	float fvTexcOut[],
	const int iFace,
	const int iVert)
{
	const MeshData* mdata = (const MeshData*)pContext->m_pUserData;
	int v = mdata->faces[iFace * 3 + iVert];
	((glm::vec2*)fvTexcOut)[0] = mdata->texcoords[v];
}

void setTSpaceBasic(
	const SMikkTSpaceContext* pContext,
	const float fvTangent[],
	const float fSign,
	const int iFace,
	const int iVert)
{
	MeshData* mdata = (MeshData*)pContext->m_pUserData;
	int v = mdata->faces[iFace * 3 + iVert];
	mdata->tangents[v] = glm::vec4(((glm::vec3*)fvTangent)[0], fSign);
}

std::vector<glm::vec4> genMikkTSpace(
	const std::vector<glm::vec3>& pos,
	const std::vector<glm::vec3>& nor,
	const std::vector<glm::vec2>& tex,
	const std::vector<unsigned int>& faces)
{
	SMikkTSpaceInterface tspace_interface;
	tspace_interface.m_getNumFaces = getNumFaces;
	tspace_interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
	tspace_interface.m_getPosition = getPosition;
	tspace_interface.m_getNormal = getNormal;
	tspace_interface.m_getTexCoord = getTexCoord;
	tspace_interface.m_setTSpaceBasic = setTSpaceBasic;
	tspace_interface.m_setTSpace = nullptr;

	MeshData mdata;

	mdata.positions = pos;
	mdata.normals = nor;
	mdata.texcoords = tex;
	mdata.faces = faces;
	mdata.tangents.resize(pos.size(), glm::vec4());

	SMikkTSpaceContext context;
	context.m_pInterface = &tspace_interface;
	context.m_pUserData = &mdata;
	if (!genTangSpaceDefault(&context))
		return std::vector<glm::vec4>();

	return mdata.tangents;
}