set(PROJECT_NAME CryCommon)

################################################################################
# Source groups
################################################################################
set(Common_h
    "_ScriptableEx.h"
    "AnimKey.h"
    "ColorDefs.h"
    "CrtDebugStats.h"
    "CryAnimationScriptCommands.h"
    "CryCharAnimationParams.h"
    "CryCharMorphParams.h"
    "CryCompiledFile.h"
    "CryEngineDecalInfo.h"
    "CryFile.h"
    "CryHeaders.h"
    "CryMemoryManager.h"
    "CryParticleSpawnInfo.h"
    "CrySizer.h"
    "CryVersion.h"
    "EntityDesc.h"
    "Font.h"
    "FrameProfiler.h"
    "IPAddress.h"
    "LeafBuffer.h"
    "list2.h"
    "MakMatInfoFromMAT_ENTITY.h"
    "MeshIdx.h"
    "Names.h"
    "primitives.h"
    "ProjectDefines.h"
    "Range.h"
    "RendElement.h"
    "ScriptObjectVector.h"
    "SerializeBuffer.h"
    "smartptr.h"
    "StlDbgAlloc.h"
    "StlUtils.h"
    "Stream.h"
    "StreamData.h"
    "StringUtils.h"
    "TAlloc.h"
    "Tarray.h"
    "TArrays.h"
    "TimeValue.h"
    "TString.h"
    "VertexBufferSource.h"
    "VertexFormats.h"
    "XboxSpecific.h"
    "XmlParserFont.h"
    "XmlStream.h"
)
source_group("Common_h" FILES ${Common_h})

set(Common_h__RenderElements
    "CRE2DQuad.h"
    "CREDummy.h"
    "CREFlashBang.h"
    "CREGlare.h"
    "CREOcclusionQuery.h"
    "CREOcLeaf.h"
    "CREPolyMesh.h"
    "CREScreenProcess.h"
    "CRESky.h"
    "CRETerrainSector.h"
    "CRETriMeshAdditionalShadow.h"
    "CRETriMeshShadow.h"
)
source_group("Common_h\\RenderElements" FILES ${Common_h__RenderElements})

set(Interfaces_h
    "AgentParams.h"
    "crysound.h"
    "I3DEngine.h"
    "I3DIndoorEngine.h"
    "IAgent.h"
    "IAISystem.h"
    "IBindable.h"
    "IBitStream.h"
    "ICompressionHelper.h"
    "IConsole.h"
    "ICryAnimation.h"
    "ICryPak.h"
    "IDataProbe.h"
    "IEdgeConnectivityBuilder.h"
    "IEntityRenderState.h"
    "IEntitySystem.h"
    "IFont.h"
    "IGame.h"
    "IInput.h"
    "ILipSync.h"
    "ILMSerializationManager.h"
    "ILog.h"
    "IMarkers.h"
    "IMiniLog.h"
    "IMovieSystem.h"
    "INetwork.h"
    "IPhysics.h"
    "IProcess.h"
    "IRenderer.h"
    "IScriptSystem.h"
    "IShader.h"
    "ISound.h"
    "IStatObj.h"
    "IStreamEngine.h"
    "IStreamPersist.h"
    "ISystem.h"
    "ITimer.h"
    "IValidator.h"
    "IXGame.h"
    "IXml.h"
    "IXMLDOM.h"
    "LMCompStructures.h"
    "physinterface.h"
)
source_group("Interfaces_h" FILES ${Interfaces_h})

set(MatLib
    "AABBSV.h"
    "Cry_Camera.h"
    "Cry_Color4.h"
    "Cry_Geo.h"
    "Cry_GeoDistance.h"
    "Cry_GeoIntersect.h"
    "Cry_GeoOverlap.h"
    "Cry_Math.h"
    "Cry_Matrix.h"
    "Cry_Quat.h"
    "Cry_Vector2.h"
    "Cry_Vector3.h"
    "Cry_XOptimise.h"
)
source_group("MatLib" FILES ${MatLib})

set(Platform_h
    "CryLibrary.h"
    "Linux32Specific.h"
    "platform.h"
    "Win32specific.h"
    "Win64specific.h"
)
source_group("Platform_h" FILES ${Platform_h})

set(ALL_FILES
    ${Common_h}
    ${Common_h__RenderElements}
    ${Interfaces_h}
    ${MatLib}
    ${Platform_h}
)

add_custom_target(${PROJECT_NAME} SOURCES ${ALL_FILES})

