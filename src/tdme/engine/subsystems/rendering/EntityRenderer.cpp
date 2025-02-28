#include <tdme/engine/subsystems/rendering/EntityRenderer.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tdme/tdme.h>
#include <tdme/engine/Texture.h>
#include <tdme/engine/model/Color4.h>
#include <tdme/engine/model/Color4Base.h>
#include <tdme/engine/model/Face.h>
#include <tdme/engine/model/FacesEntity.h>
#include <tdme/engine/model/Material.h>
#include <tdme/engine/model/Model.h>
#include <tdme/engine/model/Node.h>
#include <tdme/engine/model/PBRMaterialProperties.h>
#include <tdme/engine/model/SpecularMaterialProperties.h>
#include <tdme/engine/model/TextureCoordinate.h>
#include <tdme/engine/physics/CollisionDetection.h>
#include <tdme/engine/subsystems/lighting/LightingShader.h>
#include <tdme/engine/subsystems/lighting/LightingShaderConstants.h>
#include <tdme/engine/subsystems/lines/LinesShader.h>
#include <tdme/engine/subsystems/manager/VBOManager.h>
#include <tdme/engine/subsystems/manager/VBOManager_VBOManaged.h>
#include <tdme/engine/subsystems/particlesystem/FogParticleSystemInternal.h>
#include <tdme/engine/subsystems/particlesystem/ParticlesShader.h>
#include <tdme/engine/subsystems/particlesystem/ParticleEmitter.h>
#include <tdme/engine/subsystems/particlesystem/PointsParticleSystemInternal.h>
#include <tdme/engine/subsystems/renderer/Renderer.h>
#include <tdme/engine/subsystems/rendering/BatchRendererPoints.h>
#include <tdme/engine/subsystems/rendering/BatchRendererTriangles.h>
#include <tdme/engine/subsystems/rendering/EntityRenderer_InstancedRenderFunctionParameters.h>
#include <tdme/engine/subsystems/rendering/EntityRenderer_TransparentRenderFacesGroupPool.h>
#include <tdme/engine/subsystems/rendering/ObjectBase.h>
#include <tdme/engine/subsystems/rendering/ObjectBuffer.h>
#include <tdme/engine/subsystems/rendering/ObjectNode.h>
#include <tdme/engine/subsystems/rendering/ObjectNodeMesh.h>
#include <tdme/engine/subsystems/rendering/ObjectNodeRenderer.h>
#include <tdme/engine/subsystems/rendering/RenderTransparentRenderPointsPool.h>
#include <tdme/engine/subsystems/rendering/TransparentRenderFace.h>
#include <tdme/engine/subsystems/rendering/TransparentRenderFacesGroup.h>
#include <tdme/engine/subsystems/rendering/TransparentRenderFacesPool.h>
#include <tdme/engine/subsystems/rendering/TransparentRenderPoint.h>
#include <tdme/engine/subsystems/rendering/TransparentRenderPointsPool.h>
#include <tdme/engine/subsystems/shadowmapping/ShadowMapping.h>
#include <tdme/engine/Camera.h>
#include <tdme/engine/Engine.h>
#include <tdme/engine/Entity.h>
#include <tdme/engine/EntityHierarchy.h>
#include <tdme/engine/EnvironmentMapping.h>
#include <tdme/engine/FogParticleSystem.h>
#include <tdme/engine/Lines.h>
#include <tdme/engine/Object.h>
#include <tdme/engine/PointsParticleSystem.h>
#include <tdme/math/Math.h>
#include <tdme/math/Matrix4x4.h>
#include <tdme/math/Matrix4x4Negative.h>
#include <tdme/math/Vector2.h>
#include <tdme/math/Vector3.h>
#include <tdme/os/threading/Thread.h>
#include <tdme/utilities/ByteBuffer.h>
#include <tdme/utilities/Console.h>
#include <tdme/utilities/FloatBuffer.h>
#include <tdme/utilities/Pool.h>

using std::find;
using std::map;
using std::set;
using std::sort;
using std::string;
using std::to_string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using tdme::engine::Texture;
using tdme::engine::model::Color4;
using tdme::engine::model::Color4Base;
using tdme::engine::model::Face;
using tdme::engine::model::FacesEntity;
using tdme::engine::model::Material;
using tdme::engine::model::Model;
using tdme::engine::model::Node;
using tdme::engine::model::PBRMaterialProperties;
using tdme::engine::model::SpecularMaterialProperties;
using tdme::engine::model::TextureCoordinate;
using tdme::engine::physics::CollisionDetection;
using tdme::engine::subsystems::lighting::LightingShader;
using tdme::engine::subsystems::lighting::LightingShaderConstants;
using tdme::engine::subsystems::lines::LinesShader;
using tdme::engine::subsystems::manager::VBOManager;
using tdme::engine::subsystems::manager::VBOManager_VBOManaged;
using tdme::engine::subsystems::particlesystem::FogParticleSystemInternal;
using tdme::engine::subsystems::particlesystem::ParticlesShader;
using tdme::engine::subsystems::particlesystem::ParticleEmitter;
using tdme::engine::subsystems::particlesystem::PointsParticleSystemInternal;
using tdme::engine::subsystems::renderer::Renderer;
using tdme::engine::subsystems::rendering::BatchRendererPoints;
using tdme::engine::subsystems::rendering::BatchRendererTriangles;
using tdme::engine::subsystems::rendering::EntityRenderer;
using tdme::engine::subsystems::rendering::EntityRenderer_InstancedRenderFunctionParameters;
using tdme::engine::subsystems::rendering::EntityRenderer_TransparentRenderFacesGroupPool;
using tdme::engine::subsystems::rendering::ObjectBase;
using tdme::engine::subsystems::rendering::ObjectBuffer;
using tdme::engine::subsystems::rendering::ObjectNode;
using tdme::engine::subsystems::rendering::ObjectNodeMesh;
using tdme::engine::subsystems::rendering::ObjectNodeRenderer;
using tdme::engine::subsystems::rendering::RenderTransparentRenderPointsPool;
using tdme::engine::subsystems::rendering::TransparentRenderFace;
using tdme::engine::subsystems::rendering::TransparentRenderFacesGroup;
using tdme::engine::subsystems::rendering::TransparentRenderFacesPool;
using tdme::engine::subsystems::rendering::TransparentRenderPoint;
using tdme::engine::subsystems::rendering::TransparentRenderPointsPool;
using tdme::engine::subsystems::shadowmapping::ShadowMapping;
using tdme::engine::Engine;
using tdme::engine::Entity;
using tdme::engine::EntityHierarchy;
using tdme::engine::EnvironmentMapping;
using tdme::engine::FogParticleSystem;
using tdme::engine::Lines;
using tdme::engine::Object;
using tdme::engine::PointsParticleSystem;
using tdme::math::Math;
using tdme::math::Matrix4x4;
using tdme::math::Matrix4x4Negative;
using tdme::math::Vector3;
using tdme::os::threading::Thread;
using tdme::utilities::ByteBuffer;
using tdme::utilities::Console;
using tdme::utilities::FloatBuffer;
using tdme::utilities::Pool;

constexpr int32_t EntityRenderer::BATCHRENDERER_MAX;
constexpr int32_t EntityRenderer::INSTANCEDRENDERING_OBJECTS_MAX;

EntityRenderer::EntityRenderer(Engine* engine, Renderer* renderer) {
	this->engine = engine;
	this->renderer = renderer;
	transparentRenderFacesGroupPool = new EntityRenderer_TransparentRenderFacesGroupPool();
	transparentRenderFacesPool = new TransparentRenderFacesPool();
	renderTransparentRenderPointsPool = new RenderTransparentRenderPointsPool(65535);
	psePointBatchRenderer = new BatchRendererPoints(renderer, 0);
	threadCount = renderer->isSupportingMultithreadedRendering() == true?Engine::getThreadCount():1;
	contexts.resize(threadCount);
	if (this->renderer->isInstancedRenderingAvailable() == true) {
		for (auto& contextIdx: contexts) {
			contextIdx.bbEffectColorMuls = ByteBuffer::allocate(4 * sizeof(float) * INSTANCEDRENDERING_OBJECTS_MAX);
			contextIdx.bbEffectColorAdds = ByteBuffer::allocate(4 * sizeof(float) * INSTANCEDRENDERING_OBJECTS_MAX);
			contextIdx.bbMvMatrices = ByteBuffer::allocate(16 * sizeof(float) * INSTANCEDRENDERING_OBJECTS_MAX);
		}
	}
}

EntityRenderer::~EntityRenderer() {
	if (this->renderer->isInstancedRenderingAvailable() == true) {
		for (auto& contextIdx: contexts) {
			delete contextIdx.bbEffectColorMuls;
			delete contextIdx.bbEffectColorAdds;
			delete contextIdx.bbMvMatrices;
		}
	}
	for (auto batchRenderer: trianglesBatchRenderers) {
		delete batchRenderer;
	}
	delete transparentRenderFacesGroupPool;
	delete transparentRenderFacesPool;
	delete renderTransparentRenderPointsPool;
	delete psePointBatchRenderer;
}

void EntityRenderer::initialize()
{
	psePointBatchRenderer->initialize();
	for (auto i = 0; i < threadCount; i++) {
		auto created = false;
		auto vboManaged = Engine::getInstance()->getVBOManager()->addVBO("tdme.objectrenderer.instancedrendering." + to_string(i), 3, false, false, created);
		contexts[i].vboInstancedRenderingIds = vboManaged->getVBOIds();
	}
}

void EntityRenderer::dispose()
{
	// dispose VBOs
	for (auto i = 0; i < threadCount; i++) {
		Engine::getInstance()->getVBOManager()->removeVBO("tdme.objectrenderer.instancedrendering." + to_string(i));
	}
	// dispose batch vbo renderer
	for (auto batchRenderer: trianglesBatchRenderers) {
		batchRenderer->dispose();
		batchRenderer->release();
	}
	psePointBatchRenderer->dispose();
}

BatchRendererTriangles* EntityRenderer::acquireTrianglesBatchRenderer()
{
	// check for free batch vbo renderer
	auto i = 0;
	for (auto batchRenderer: trianglesBatchRenderers) {
		if (batchRenderer->acquire()) return batchRenderer;
		i++;
	}
	// try to add one
	if (i < BATCHRENDERER_MAX) {
		auto batchRenderer = new BatchRendererTriangles(renderer, i);
		batchRenderer->initialize();
		trianglesBatchRenderers.push_back(batchRenderer);
		if (batchRenderer->acquire()) return batchRenderer;

	}
	Console::println(string("EntityRenderer::acquireTrianglesBatchRenderer()::failed"));
	// nope
	return nullptr;
}

void EntityRenderer::reset()
{
	objectsByShadersAndModels.clear();
}

void EntityRenderer::render(Entity::RenderPass renderPass, const vector<Object*>& objects, bool renderTransparentFaces, int32_t renderTypes)
{
	if (renderer->isSupportingMultithreadedRendering() == false) {
		renderFunction(0, renderPass, objects, objectsByShadersAndModels, renderTransparentFaces, renderTypes, transparentRenderFacesPool);
	} else {
		auto elementsIssued = 0;
		auto queueElement = Engine::engineThreadQueueElementPool.allocate();
		queueElement->type = Engine::EngineThreadQueueElement::TYPE_RENDERING;
		queueElement->engine = engine;
		queueElement->rendering.renderPass = renderPass;
		queueElement->rendering.collectTransparentFaces = renderTransparentFaces;
		queueElement->rendering.renderTypes = renderTypes;
		for (auto i = 0; i < objects.size(); i++) {
			queueElement->objects.push_back(objects[i]);
			if (queueElement->objects.size() == Engine::ENGINETHREADSQUEUE_RENDER_DISPATCH_COUNT) {
				auto queueElementToSubmit = queueElement;
				queueElement = Engine::engineThreadQueueElementPool.allocate();
				queueElement->type = Engine::EngineThreadQueueElement::TYPE_RENDERING;
				queueElement->engine = engine;
				queueElement->rendering.renderPass = renderPass;
				queueElement->rendering.collectTransparentFaces = renderTransparentFaces;
				queueElement->rendering.renderTypes = renderTypes;
				elementsIssued++;
				engine->engineThreadsQueue->addElement(queueElementToSubmit, false);
			}
		}
		if (queueElement->objects.empty() == false) {
			elementsIssued++;
			engine->engineThreadsQueue->addElement(queueElement, false);
		}

		// wait until all elements have been processed
		while (true == true) {
			auto elementsProcessed = 0;
			for (auto engineThread: Engine::engineThreads) elementsProcessed+= engineThread->getProcessedElements();
			if (elementsProcessed == elementsIssued) {
				for (auto engineThread: Engine::engineThreads) engineThread->resetProcessedElements();
				break;
			}
		}

		//
		Engine::engineThreadQueueElementPool.reset();

		//
		for (auto engineThread: Engine::engineThreads) {
			transparentRenderFacesPool->merge(engineThread->transparentRenderFacesPool);
			engineThread->transparentRenderFacesPool->reset();
		}
	}
}

void EntityRenderer::renderTransparentFaces() {
	// use default context
	auto contextIdx = renderer->CONTEXTINDEX_DEFAULT;
	// render transparent render faces if any exist
	auto& transparentRenderFaces = transparentRenderFacesPool->getTransparentRenderFaces();
	if (transparentRenderFaces.size() > 0) {
		// sort transparent render faces from far to near
		sort(transparentRenderFaces.begin(), transparentRenderFaces.end(), TransparentRenderFace::compare);
		// second render pass, draw color buffer for transparent objects
		// 	set up blending, but no culling and no depth buffer
		//	TODO: enabling depth buffer let shadow disappear
		//renderer->disableDepthBufferWriting();  // TODO: a.drewke, verify that this works ok in all cases?
		renderer->disableCulling(contextIdx);
		renderer->enableBlending();
		// disable foliage animation
		// reset shader
		// TODO: shader parameters
		renderer->setShader(contextIdx, string());
		renderer->onUpdateShader(contextIdx);
		// have identity texture matrix
		renderer->getTextureMatrix(contextIdx).identity();
		renderer->onUpdateTextureMatrix(contextIdx);
		// actually this should not make any difference as culling is disabled
		// but having a fixed value is not a bad idea except that it is a renderer call
		// TODO: confirm this
		renderer->setFrontFace(contextIdx, renderer->FRONTFACE_CCW);
		for (auto transparentRenderFace: transparentRenderFaces) {
			// do we have any faces yet?
			if (nodeTransparentRenderFaces.size() == 0) {
				// nope, so add this one
				nodeTransparentRenderFaces.push_back(transparentRenderFace);
			} else
			// do we have more than face already?
			if (nodeTransparentRenderFaces.size() > 0) {
				// check if we have more of first type
				if (nodeTransparentRenderFaces[0]->objectNode == transparentRenderFace->objectNode) {
					// yep, we can add this one
					nodeTransparentRenderFaces.push_back(transparentRenderFace);
				} else {
					// no, render nodeed faces
					prepareTransparentFaces(nodeTransparentRenderFaces);
					// reset
					nodeTransparentRenderFaces.clear();
					// add current face
					nodeTransparentRenderFaces.push_back(transparentRenderFace);
				}
			}
		}
		// 	check if there are any left overs
		if (nodeTransparentRenderFaces.size() > 0) {
			prepareTransparentFaces(nodeTransparentRenderFaces);
			nodeTransparentRenderFaces.clear();
		}
		// render transparent faces nodes
		renderTransparentFacesGroups(contextIdx);
		//	no blending, but culling and depth buffer
		renderer->disableBlending();
		renderer->enableCulling(contextIdx);
		//renderer->enableDepthBufferWriting(); // TODO: a.drewke, verify that this works ok in all cases?
		// done!
	}

	// clear transparent render faces data
	transparentRenderFacesPool->reset();
	releaseTransparentFacesGroups();
}

void EntityRenderer::prepareTransparentFaces(const vector<TransparentRenderFace*>& transparentRenderFaces)
{
	// all those faces should share the object and object node, ...
	auto objectNode = transparentRenderFaces[0]->objectNode;
	auto object = static_cast<Object*>(objectNode->object);
	// model view matrix to be used with given transparent render faces
	Matrix4x4 modelViewMatrix;
	if (objectNode->mesh->skinning == true) {
		modelViewMatrix.identity();
	} else {
		modelViewMatrix.set(*objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix());
	}
	//
	auto model = objectNode->object->getModel();
	auto& facesEntities = objectNode->node->getFacesEntities();
	const FacesEntity* facesEntity = nullptr;
	// attributes we collect for a transparent render face node
	auto& effectColorAdd = object->getEffectColorAdd();
	auto& effectColorMul = object->getEffectColorMul();
	const Material* material = nullptr;
	auto textureCoordinates = false;
	// render transparent faces
	for (auto i = 0; i < transparentRenderFaces.size(); i++) {
		auto transparentRenderFace = transparentRenderFaces[i];
		auto facesEntityIdx = transparentRenderFace->facesEntityIdx;
		// determine if faces entity and so material did switch between last face and current face
		if (facesEntity != &facesEntities[facesEntityIdx]) {
			facesEntity = &facesEntities[facesEntityIdx];
			material = facesEntity->getMaterial();
		}
		textureCoordinates = facesEntity->isTextureCoordinatesAvailable();
		// create node key
		auto transparentRenderFacesNodeKey = TransparentRenderFacesGroup::createKey(model, objectNode, facesEntityIdx, effectColorAdd, effectColorMul, material, textureCoordinates, object->getShader());
		// get node
		TransparentRenderFacesGroup* trfNode = nullptr;
		auto trfNodeIt = transparentRenderFacesGroups.find(transparentRenderFacesNodeKey);
		if (trfNodeIt != transparentRenderFacesGroups.end()) {
			trfNode = trfNodeIt->second;
		}
		if (trfNode == nullptr) {
			// we do not have the node, create node
			trfNode = transparentRenderFacesGroupPool->allocate();
			trfNode->set(this, model, objectNode, facesEntityIdx, effectColorAdd, effectColorMul, material, textureCoordinates, object->getShader());
			transparentRenderFacesGroups[transparentRenderFacesNodeKey] = trfNode;
		}
		auto& textureCoordinates = transparentRenderFace->objectNode->mesh->node->getTextureCoordinates();
		for (auto vertexIdx = 0; vertexIdx < 3; vertexIdx++) {
			auto arrayIdx = transparentRenderFace->objectNode->mesh->indices[transparentRenderFace->faceIdx * 3 + vertexIdx];
			trfNode->addVertex(
				modelViewMatrix.multiply((*transparentRenderFace->objectNode->mesh->vertices)[arrayIdx]),
				modelViewMatrix.multiplyNoTranslation((*transparentRenderFace->objectNode->mesh->normals)[arrayIdx]),
				transparentRenderFace->objectNode->textureMatricesByEntities[facesEntityIdx].multiply(
					textureCoordinates.size() > 0?
						Vector2(textureCoordinates[arrayIdx].getArray()):
						Vector2(0.0f, 0.0f)
				)
			);
		}
	}
}

void EntityRenderer::renderTransparentFacesGroups(int contextIdx) {
	for (auto& it: transparentRenderFacesGroups) {
		it.second->render(engine, renderer, contextIdx);
	}
}

void EntityRenderer::releaseTransparentFacesGroups()
{
	for (auto& it: transparentRenderFacesGroups) {
		transparentRenderFacesGroupPool->release(it.second);
	}
	transparentRenderFacesGroups.clear();
}

void EntityRenderer::renderObjectsOfSameTypeNonInstanced(const vector<Object*>& objects, bool collectTransparentFaces, int32_t renderTypes) {
	Vector3 objectCamFromAxis;
	auto camera = engine->getCamera();

	// use default context
	auto& objectRenderContext = contexts[0];
	auto contextIdx = renderer->CONTEXTINDEX_DEFAULT;

	//
	auto shadowMapping = engine->getShadowMapping();
	Matrix4x4 modelViewMatrix;
	Matrix4x4 cameraMatrix;
	cameraMatrix.set(renderer->getModelViewMatrix());
	// render faces entities
	auto currentFrontFace = -1;
	string shaderParametersHash;
	auto firstObject = objects[0];
	// all objects share the same object node structure, so we just take the first one
	vector<int32_t>* boundVBOBaseIds = nullptr;
	vector<int32_t>* boundVBOTangentBitangentIds = nullptr;
	vector<int32_t>* boundVBOOrigins = nullptr;
	auto currentLODLevel = -1;
	int32_t boundEnvironmentMappingCubeMapTextureId = -1;
	Vector3 boundEnvironmentMappingCubeMapPosition;
	for (auto objectNodeIdx = 0; objectNodeIdx < firstObject->objectNodes.size(); objectNodeIdx++) {
		auto objectNode = firstObject->objectNodes[objectNodeIdx];
		// render each faces entity
		auto& facesEntities = objectNode->node->getFacesEntities();
		auto faceIdx = 0;
		auto facesEntityIdxCount = facesEntities.size();
		for (auto faceEntityIdx = 0; faceEntityIdx < facesEntityIdxCount; faceEntityIdx++) {
			auto facesEntity = &facesEntities[faceEntityIdx];
			auto isTextureCoordinatesAvailable = facesEntity->isTextureCoordinatesAvailable();
			auto faces = facesEntity->getFaces().size() * firstObject->instances;
			auto facesToRender = facesEntity->getFaces().size() * firstObject->enabledInstances;
			// material
			auto material = facesEntity->getMaterial();
			auto specularMaterialProperties = material != nullptr?material->getSpecularMaterialProperties():nullptr;
			// determine if transparent
			auto transparentFacesEntity = false;
			//	via material
			if (specularMaterialProperties != nullptr) {
				if (specularMaterialProperties->hasColorTransparency() == true || specularMaterialProperties->hasTextureTransparency() == true) transparentFacesEntity = true;
				if (material->isDoubleSided() == true ||
					(specularMaterialProperties->hasDiffuseTextureTransparency() == true && specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true)) {
					renderer->disableCulling(contextIdx);
				}
			}
			// skip, if requested
			if (transparentFacesEntity == true) {
				// add to transparent render faces, if requested
				auto objectCount = objects.size();
				for (auto objectIdx = 0; objectIdx < objectCount; objectIdx++) {
					auto object = objects[objectIdx];
					auto _objectNode = object->objectNodes[objectNodeIdx];
					// set up textures
					ObjectNode::setupTextures(renderer, contextIdx, objectNode, faceEntityIdx);
					// set up transparent render faces
					if (collectTransparentFaces == true) {
						transparentRenderFacesPool->createTransparentRenderFaces(
							(_objectNode->mesh->skinning == true?
								modelViewMatrix.identity():
								modelViewMatrix.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
							).multiply(cameraMatrix),
							object->objectNodes[objectNodeIdx],
							faceEntityIdx,
							faceIdx
						);
					}
				}
				// keep track of rendered faces
				faceIdx += faces;
				// skip to next entity
				continue;
			}
			// draw this faces entity for each object
			bool materialUpdateOnly = false;
			auto objectCount = objects.size();
			for (auto objectIdx = 0; objectIdx < objectCount; objectIdx++) {
				auto object = objects[objectIdx];
				auto _objectNode = object->objectNodes[objectNodeIdx];
				//	check transparency via effect
				if (object->effectColorMul.getAlpha() < 1.0f - Math::EPSILON ||
					object->effectColorAdd.getAlpha() < -Math::EPSILON) {
					// add to transparent render faces, if requested
					if (collectTransparentFaces == true) {
						transparentRenderFacesPool->createTransparentRenderFaces(
							(_objectNode->mesh->skinning == true ?
								modelViewMatrix.identity() :
								modelViewMatrix.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
							).multiply(cameraMatrix),
							_objectNode,
							faceEntityIdx,
							faceIdx
						);
					}
					// skip to next object
					continue;
				}

				// shader
				// TODO: shader parameters
				if (renderer->getShader(contextIdx) != object->getShader()) {
					renderer->setShader(contextIdx, object->getShader());
					renderer->onUpdateShader(contextIdx);
					// update lights
					for (auto j = 0; j < engine->lights.size(); j++) engine->lights[j].update(contextIdx);
					materialUpdateOnly = false;
				}
				// set up material on first object
				string materialKey;
				if (materialUpdateOnly == false || checkMaterialChangable(_objectNode, faceEntityIdx, renderTypes) == true) {
					setupMaterial(contextIdx, _objectNode, faceEntityIdx, renderTypes, materialUpdateOnly, materialKey);
					// only update materials for next calls
					materialUpdateOnly = true;
				}
				// shader parameters
				if (shaderParametersHash.empty() == true || shaderParametersHash != renderer->getShaderParameters(contextIdx).getShaderParametersHash()) {
					renderer->setShaderParameters(contextIdx, object->shaderParameters);
					renderer->onUpdateShaderParameters(contextIdx);
					shaderParametersHash = renderer->getShaderParameters(contextIdx).getShaderParametersHash();
				}
				// bind buffer base objects if not bound yet
				auto currentVBOBaseIds = _objectNode->renderer->vboBaseIds;
				if (boundVBOBaseIds != currentVBOBaseIds) {
					boundVBOBaseIds = currentVBOBaseIds;
					//	texture coordinates
					if (isTextureCoordinatesAvailable == true &&
						(((renderTypes & RENDERTYPE_TEXTUREARRAYS) == RENDERTYPE_TEXTUREARRAYS) ||
						((renderTypes & RENDERTYPE_TEXTUREARRAYS_DIFFUSEMASKEDTRANSPARENCY) == RENDERTYPE_TEXTUREARRAYS_DIFFUSEMASKEDTRANSPARENCY && specularMaterialProperties != nullptr && specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true))) {
						renderer->bindTextureCoordinatesBufferObject(contextIdx, (*currentVBOBaseIds)[3]);
					}
					// 	indices
					renderer->bindIndicesBufferObject(contextIdx, (*currentVBOBaseIds)[0]);
					// 	vertices
					renderer->bindVerticesBufferObject(contextIdx, (*currentVBOBaseIds)[1]);
					// 	normals
					if ((renderTypes & RENDERTYPE_NORMALS) == RENDERTYPE_NORMALS) renderer->bindNormalsBufferObject(contextIdx, (*currentVBOBaseIds)[2]);
				}
				auto currentVBOLods = _objectNode->renderer->vboLods;
				if (currentVBOLods != nullptr) {
					auto distanceSquared = objectCamFromAxis.set(object->getBoundingBoxTransformed()->computeClosestPointInBoundingBox(camera->getLookFrom())).sub(camera->getLookFrom()).computeLengthSquared();
					// index buffer
					auto lodLevel = 0;
					if (currentVBOLods->size() >= 3 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Distance())) {
						lodLevel = 3;
					} else
					if (currentVBOLods->size() >= 2 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Distance())) {
						lodLevel = 2;
					} else
					if (currentVBOLods->size() >= 1 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Distance())) {
						lodLevel = 1;
					}
					if (lodLevel == 0) {
						renderer->bindIndicesBufferObject(contextIdx, (*currentVBOBaseIds)[0]);
					} else {
						renderer->bindIndicesBufferObject(contextIdx, (*currentVBOLods)[lodLevel - 1]);
						switch(lodLevel) {
							case 3:
								faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Indices().size() / 3) * firstObject->instances;
								facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Indices().size() / 3) * firstObject->enabledInstances;
								break;
							case 2:
								faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Indices().size() / 3) * firstObject->instances;
								facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Indices().size() / 3) * firstObject->enabledInstances;
								break;
							case 1:
								faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Indices().size() / 3) * firstObject->instances;
								facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Indices().size() / 3) * firstObject->enabledInstances;
								break;
							default: break;
						}
					}
					currentLODLevel = lodLevel;
				}
				// bind tangent, bitangend buffers if not yet bound
				auto currentVBONormalMappingIds = _objectNode->renderer->vboNormalMappingIds;
				if ((renderTypes & RENDERTYPE_NORMALS) == RENDERTYPE_NORMALS &&
					renderer->isNormalMappingAvailable() && currentVBONormalMappingIds != nullptr && currentVBONormalMappingIds != boundVBOTangentBitangentIds) {
					// tangent
					renderer->bindTangentsBufferObject(contextIdx, (*currentVBONormalMappingIds)[0]);
					// bitangent
					renderer->bindBitangentsBufferObject(contextIdx, (*currentVBONormalMappingIds)[1]);
				}
				// bind render node object origins
				auto currentVBOOrigins = _objectNode->renderer->vboOrigins;
				if (currentVBOOrigins != nullptr && currentVBOOrigins != boundVBOOrigins) {
					renderer->bindOriginsBufferObject(contextIdx, (*currentVBOOrigins)[0]);
				}
				// set up local -> world transform matrix
				renderer->getModelViewMatrix().set(
					_objectNode->mesh->skinning == true?
						modelViewMatrix.identity():
						modelViewMatrix.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
				);
				renderer->onUpdateModelViewMatrix(contextIdx);
				// set up front face
				auto objectFrontFace = objectRenderContext.matrix4x4Negative.isNegative(renderer->getModelViewMatrix()) == false ? renderer->FRONTFACE_CCW : renderer->FRONTFACE_CW;
				if (objectFrontFace != currentFrontFace) {
					renderer->setFrontFace(contextIdx, objectFrontFace);
					currentFrontFace = objectFrontFace;
				}
				// set up effect color
				if ((renderTypes & RENDERTYPE_EFFECTCOLORS) == RENDERTYPE_EFFECTCOLORS) {
					renderer->getEffectColorMul(contextIdx) = object->effectColorMul.getArray();
					renderer->getEffectColorAdd(contextIdx) = object->effectColorAdd.getArray();
					renderer->onUpdateEffect(contextIdx);
				}
				// do transform start to shadow mapping
				if ((renderTypes & RENDERTYPE_SHADOWMAPPING) == RENDERTYPE_SHADOWMAPPING &&
					shadowMapping != nullptr) {
					shadowMapping->startObjectTransform(
						contextIdx,
						_objectNode->mesh->skinning == true ?
							modelViewMatrix.identity() :
							modelViewMatrix.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
					);
				}
				// set up texture matrix
				//	TODO: check if texture is in use
				if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES ||
					(renderTypes & RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY) == RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY) {
					renderer->getTextureMatrix(contextIdx).set(_objectNode->textureMatricesByEntities[faceEntityIdx]);
					renderer->onUpdateTextureMatrix(contextIdx);
				}
				EnvironmentMapping* environmentMappingEntity = nullptr;
				// reflection source
				if (object->getReflectionEnvironmentMappingId().empty() == false) {
					auto environmentMappingEntityCandidate = engine->getEntity(object->getReflectionEnvironmentMappingId());
					if (environmentMappingEntityCandidate != nullptr) {
						if (environmentMappingEntityCandidate->getEntityType() == Entity::ENTITYTYPE_ENVIRONMENTMAPPING) {
							environmentMappingEntity = static_cast<EnvironmentMapping*>(environmentMappingEntityCandidate);
						} else
						if (environmentMappingEntityCandidate->getEntityType() == Entity::ENTITYTYPE_ENTITYHIERARCHY) {
							auto entityHierarchyEnvironmentMappingEntity = static_cast<EntityHierarchy*>(environmentMappingEntityCandidate)->getEntityByType(Entity::ENTITYTYPE_ENVIRONMENTMAPPING);
							if (entityHierarchyEnvironmentMappingEntity != nullptr) environmentMappingEntity = static_cast<EnvironmentMapping*>(entityHierarchyEnvironmentMappingEntity);
						}
					}
					if (environmentMappingEntity != nullptr) {
						Vector3 environmentMappingTranslation;
						object->getTransformMatrix().getTranslation(environmentMappingTranslation);
						auto environmentMappingCubeMapTextureId = environmentMappingEntity->getCubeMapTextureId();
						Vector3 environmentMappingCubeMapPosition = object->hasReflectionEnvironmentMappingPosition() == true?object->getReflectionEnvironmentMappingPosition():environmentMappingTranslation;
						if (environmentMappingCubeMapTextureId != boundEnvironmentMappingCubeMapTextureId || environmentMappingCubeMapPosition.equals(boundEnvironmentMappingCubeMapPosition) == false) {
							boundEnvironmentMappingCubeMapTextureId = environmentMappingCubeMapTextureId;
							boundEnvironmentMappingCubeMapPosition = environmentMappingCubeMapPosition;
							renderer->setEnvironmentMappingCubeMapPosition(contextIdx, environmentMappingCubeMapPosition.getArray());
							renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_ENVIRONMENT);
							renderer->bindCubeMapTexture(contextIdx, environmentMappingCubeMapTextureId);
							renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
						}
					}
				}
				if (environmentMappingEntity == nullptr && boundEnvironmentMappingCubeMapTextureId != -1) {
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_ENVIRONMENT);
					renderer->bindCubeMapTexture(contextIdx, renderer->ID_NONE);
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
					boundEnvironmentMappingCubeMapTextureId = -1;
				}
				// draw
				renderer->drawIndexedTrianglesFromBufferObjects(contextIdx, facesToRender, faceIdx);
				// do transform end to shadow mapping
				if ((renderTypes & RENDERTYPE_SHADOWMAPPING) == RENDERTYPE_SHADOWMAPPING &&
					shadowMapping != nullptr) {
					shadowMapping->endObjectTransform();
				}
			}
			// keep track of rendered faces
			faceIdx += faces;
			if (specularMaterialProperties != nullptr) {
				if (material->isDoubleSided() == true ||
					(specularMaterialProperties->hasDiffuseTextureTransparency() == true && specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true)) {
					renderer->enableCulling(contextIdx);
				}
			}
		}
	}
	// unbind buffers
	renderer->unbindBufferObjects(contextIdx);
	// restore model view matrix / view matrix
	renderer->getModelViewMatrix().set(cameraMatrix);
}

void EntityRenderer::renderObjectsOfSameTypeInstanced(int threadIdx, const vector<Object*>& objects, bool collectTransparentFaces, int32_t renderTypes, TransparentRenderFacesPool* transparentRenderFacesPool)
{
	// contexts
	auto& objectRenderContext = contexts[threadIdx];
	auto contextIdx = threadIdx;

	//
	auto cameraMatrix = renderer->getCameraMatrix();
	Vector3 objectCamFromAxis;
	Matrix4x4 modelViewMatrixTemp;
	Matrix4x4 modelViewMatrix;

	//
	auto camera = engine->camera;
	auto frontFace = -1;
	auto cullingMode = 1;

	// render faces entities
	auto firstObject = objects[0];

	// all objects share the same object node structure, so we just take the first one
	for (auto objectNodeIdx = 0; objectNodeIdx < firstObject->objectNodes.size(); objectNodeIdx++) {
		auto objectNode = firstObject->objectNodes[objectNodeIdx];
		// render each faces entity
		auto& facesEntities = objectNode->node->getFacesEntities();
		auto faceIdx = 0;
		auto facesEntityIdxCount = facesEntities.size();
		for (auto faceEntityIdx = 0; faceEntityIdx < facesEntityIdxCount; faceEntityIdx++) {
			auto facesEntity = &facesEntities[faceEntityIdx];
			auto isTextureCoordinatesAvailable = facesEntity->isTextureCoordinatesAvailable();
			auto faces = facesEntity->getFaces().size() * firstObject->instances;
			auto facesToRender = facesEntity->getFaces().size() * firstObject->enabledInstances;
			// material
			auto material = facesEntity->getMaterial();
			auto specularMaterialProperties = material != nullptr?material->getSpecularMaterialProperties():nullptr;
			// determine if transparent
			auto transparentFacesEntity = false;
			//	via material
			if (specularMaterialProperties != nullptr) {
				if (specularMaterialProperties->hasColorTransparency() == true || specularMaterialProperties->hasTextureTransparency() == true) transparentFacesEntity = true;

				// skip, if requested
				if (transparentFacesEntity == true) {
					// add to transparent render faces, if requested
					auto objectCount = objects.size();
					for (auto objectIdx = 0; objectIdx < objectCount; objectIdx++) {
						auto object = objects[objectIdx];
						auto _objectNode = object->objectNodes[objectNodeIdx];
						// set up textures
						ObjectNode::setupTextures(renderer, contextIdx, objectNode, faceEntityIdx);
						// set up transparent render faces
						if (collectTransparentFaces == true) {
							transparentRenderFacesPool->createTransparentRenderFaces(
								(_objectNode->mesh->skinning == true?
									modelViewMatrixTemp.identity() :
									modelViewMatrixTemp.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
								).multiply(cameraMatrix),
								object->objectNodes[objectNodeIdx],
								faceEntityIdx,
								faceIdx
							);
						}
					}
					// keep track of rendered faces
					faceIdx += faces;
					// skip to next entity
					continue;
				}

				if (material->isDoubleSided() == true ||
					(specularMaterialProperties->hasDiffuseTextureTransparency() == true && specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true)) {
					if (cullingMode != 0) {
						renderer->disableCulling(contextIdx);
						cullingMode = 0;
					}
				} else {
					if (cullingMode != 1) {
						renderer->enableCulling(contextIdx);
						cullingMode = 1;
					}
				}
			} else {
				if (cullingMode != 1) {
					renderer->enableCulling(contextIdx);
					cullingMode = 1;
				}
			}

			// draw this faces entity for each object
			objectRenderContext.objectsToRender = objects;
			do {
				auto hadFrontFaceSetup = false;
				auto hadShaderSetup = false;
				Matrix4x4Negative matrix4x4Negative;

				Vector3 objectCamFromAxis;
				Matrix4x4 modelViewMatrixTemp;
				Matrix4x4 modelViewMatrix;

				FloatBuffer fbEffectColorMuls = objectRenderContext.bbEffectColorMuls->asFloatBuffer();
				FloatBuffer fbEffectColorAdds = objectRenderContext.bbEffectColorAdds->asFloatBuffer();
				FloatBuffer fbMvMatrices = objectRenderContext.bbMvMatrices->asFloatBuffer();

				string materialKey;
				string shaderParametersHash;
				bool materialUpdateOnly = false;
				vector<int32_t>* boundVBOBaseIds = nullptr;
				vector<int32_t>* boundVBOTangentBitangentIds = nullptr;
				vector<int32_t>* boundVBOOrigins = nullptr;
				auto currentLODLevel = -1;
				int32_t boundEnvironmentMappingCubeMapTextureId = -1;
				Vector3 boundEnvironmentMappingCubeMapPosition;
				auto objectCount = objectRenderContext.objectsToRender.size();

				//
				auto textureMatrix = objectRenderContext.objectsToRender[0]->objectNodes[objectNodeIdx]->textureMatricesByEntities[faceEntityIdx];

				// draw objects
				for (auto objectIdx = 0; objectIdx < objectCount; objectIdx++) {
					auto object = objectRenderContext.objectsToRender[objectIdx];
					auto _objectNode = object->objectNodes[objectNodeIdx];

					//	check transparency via effect
					if (object->effectColorMul.getAlpha() < 1.0f - Math::EPSILON ||
						object->effectColorAdd.getAlpha() < -Math::EPSILON) {
						// add to transparent render faces, if requested
						if (collectTransparentFaces == true) {
							transparentRenderFacesPool->createTransparentRenderFaces(
								(_objectNode->mesh->skinning == true ?
									modelViewMatrixTemp.identity() :
									modelViewMatrixTemp.set(*_objectNode->nodeTransformMatrix).multiply(object->getTransformMatrix())
								).multiply(cameraMatrix),
								_objectNode,
								faceEntityIdx,
								faceIdx
							);
						}
						// skip to next object
						continue;
					}

					// limit objects to render to INSTANCEDRENDERING_OBJECTS_MAX
					if (fbMvMatrices.getPosition() / 16 == INSTANCEDRENDERING_OBJECTS_MAX) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}

					// check if texture matrix did change
					if (_objectNode->textureMatricesByEntities[faceEntityIdx].equals(textureMatrix) == false) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}

					// shader
					if (hadShaderSetup == false) {
						if (object->getShader() != renderer->getShader(contextIdx)) {
							renderer->setShader(contextIdx, object->getShader());
							renderer->onUpdateShader(contextIdx);
							for (auto j = 0; j < engine->lights.size(); j++) engine->lights[j].update(contextIdx);
							// issue upload matrices
							renderer->onUpdateCameraMatrix(contextIdx);
							renderer->onUpdateProjectionMatrix(contextIdx);
							renderer->onUpdateTextureMatrix(contextIdx);
						}
						hadShaderSetup = true;
					} else
					if (object->getShader() != renderer->getShader(contextIdx)) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}

					// set up material on first object and update on succeeding
					auto materialKeyCurrent = materialKey;
					if (materialUpdateOnly == false || checkMaterialChangable(_objectNode, faceEntityIdx, renderTypes) == true) {
						setupMaterial(contextIdx, _objectNode, faceEntityIdx, renderTypes, materialUpdateOnly, materialKeyCurrent, materialKey);
						// only update material for next material calls
						if (materialUpdateOnly == false) {
							materialKey = materialKeyCurrent;
							materialUpdateOnly = true;
						}
					}

					// check if material key has not been set yet
					if (materialKey != materialKeyCurrent) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}

					// shader parameters
					if (shaderParametersHash.empty() == true) {
						renderer->setShaderParameters(contextIdx, object->shaderParameters);
						renderer->onUpdateShaderParameters(contextIdx);
						shaderParametersHash = renderer->getShaderParameters(contextIdx).getShaderParametersHash();
					} else
					if (shaderParametersHash != renderer->getShaderParameters(contextIdx).getShaderParametersHash()) {
						objectRenderContext.objectsNotRendered.push_back(object);
					}

					// bind buffer base objects if not bound yet
					auto currentVBOBaseIds = _objectNode->renderer->vboBaseIds;
					if (boundVBOBaseIds == nullptr) {
						boundVBOBaseIds = currentVBOBaseIds;
						//	texture coordinates
						if (isTextureCoordinatesAvailable == true &&
							(((renderTypes & RENDERTYPE_TEXTUREARRAYS) == RENDERTYPE_TEXTUREARRAYS) ||
							((renderTypes & RENDERTYPE_TEXTUREARRAYS_DIFFUSEMASKEDTRANSPARENCY) == RENDERTYPE_TEXTUREARRAYS_DIFFUSEMASKEDTRANSPARENCY && specularMaterialProperties != nullptr && specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true))) {
							renderer->bindTextureCoordinatesBufferObject(contextIdx, (*currentVBOBaseIds)[3]);
						}
						// 	indices
						renderer->bindIndicesBufferObject(contextIdx, (*currentVBOBaseIds)[0]);
						// 	vertices
						renderer->bindVerticesBufferObject(contextIdx, (*currentVBOBaseIds)[1]);
						// 	normals
						if ((renderTypes & RENDERTYPE_NORMALS) == RENDERTYPE_NORMALS) {
							renderer->bindNormalsBufferObject(contextIdx, (*currentVBOBaseIds)[2]);
						}
					} else
					// check if buffers did change, then skip and render in next step
					if (boundVBOBaseIds != currentVBOBaseIds) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}
					auto currentVBOLods = _objectNode->renderer->vboLods;
					if (currentVBOLods != nullptr) {
						auto distanceSquared = objectCamFromAxis.set(object->getBoundingBoxTransformed()->computeClosestPointInBoundingBox(camera->getLookFrom())).sub(camera->getLookFrom()).computeLengthSquared();
						// index buffer
						auto lodLevel = 0;
						if (currentVBOLods->size() >= 3 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Distance())) {
							lodLevel = 3;
						} else
						if (currentVBOLods->size() >= 2 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Distance())) {
							lodLevel = 2;
						} else
						if (currentVBOLods->size() >= 1 && distanceSquared >= Math::square(_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Distance())) {
							lodLevel = 1;
						}
						if (currentLODLevel != -1 && lodLevel != currentLODLevel) {
							objectRenderContext.objectsNotRendered.push_back(object);
							continue;
						}
						if (lodLevel > 0) {
							renderer->bindIndicesBufferObject(contextIdx, (*currentVBOLods)[lodLevel - 1]);
							switch(lodLevel) {
								case 3:
									faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Indices().size() / 3) * firstObject->instances;
									facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD3Indices().size() / 3) * firstObject->enabledInstances;
									break;
								case 2:
									faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Indices().size() / 3) * firstObject->instances;
									facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD2Indices().size() / 3) * firstObject->enabledInstances;
									break;
								case 1:
									faces = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Indices().size() / 3) * firstObject->instances;
									facesToRender = (_objectNode->node->getFacesEntities()[faceEntityIdx].getLOD1Indices().size() / 3) * firstObject->enabledInstances;
									break;
								default: break;
							}
						}
						currentLODLevel = lodLevel;
					}
					// bind tangent, bitangend buffers
					auto currentVBONormalMappingIds = _objectNode->renderer->vboNormalMappingIds;
					if ((renderTypes & RENDERTYPE_NORMALS) == RENDERTYPE_NORMALS &&
						renderer->isNormalMappingAvailable() == true && currentVBONormalMappingIds != nullptr) {
						// bind tangent, bitangend buffers if not yet done
						if (boundVBOTangentBitangentIds == nullptr) {
							// tangent
							renderer->bindTangentsBufferObject(contextIdx, (*currentVBONormalMappingIds)[0]);
							// bitangent
							renderer->bindBitangentsBufferObject(contextIdx, (*currentVBONormalMappingIds)[1]);
							//
							boundVBOTangentBitangentIds = currentVBONormalMappingIds;
						} else
						// check if buffers did change, then skip and render in next step
						if (currentVBONormalMappingIds != boundVBOTangentBitangentIds) {
							objectRenderContext.objectsNotRendered.push_back(object);
							continue;
						}
					}

					// bind render node object origins
					auto currentVBOOrigins = _objectNode->renderer->vboOrigins;
					if (currentVBOOrigins != nullptr) {
						// bind render node object origins if not yet done
						if (boundVBOOrigins == nullptr) {
							renderer->bindOriginsBufferObject(contextIdx, (*currentVBOOrigins)[0]);
							//
							boundVBOOrigins = currentVBOOrigins;
						} else
						// check if buffers did change, then skip and render in next step
						if (currentVBOOrigins != boundVBOOrigins) {
							objectRenderContext.objectsNotRendered.push_back(object);
							continue;
						}
					}

					// set up local -> world transform matrix
					modelViewMatrix.set(
						_objectNode->mesh->skinning == true?
							modelViewMatrixTemp.identity() :
							modelViewMatrixTemp.set(*_objectNode->nodeTransformMatrix).
							multiply(object->getTransformMatrix())
					);

					// set up front face
					auto objectFrontFace = matrix4x4Negative.isNegative(modelViewMatrix) == false ? renderer->FRONTFACE_CCW : renderer->FRONTFACE_CW;
					// if front face changed just render in next step, this all makes only sense if culling is enabled
					if (cullingMode == 1) {
						if (hadFrontFaceSetup == false) {
							hadFrontFaceSetup = true;
							if (objectFrontFace != frontFace) {
								frontFace = objectFrontFace;
								renderer->setFrontFace(contextIdx, frontFace);
							}
						} else
						if (objectFrontFace != frontFace) {
							objectRenderContext.objectsNotRendered.push_back(object);
							continue;
						}
					}

					// reflection source
					if (object->getReflectionEnvironmentMappingId().empty() == false) {
						EnvironmentMapping* environmentMappingEntity = nullptr;
						// some note: engine->getEntity() is not thread safe if having multithreaded renderer, but at this time there should be no other access anyways
						auto environmentMappingEntityCandidate = engine->getEntity(object->getReflectionEnvironmentMappingId());
						if (environmentMappingEntityCandidate != nullptr) {
							if (environmentMappingEntityCandidate->getEntityType() == Entity::ENTITYTYPE_ENVIRONMENTMAPPING) {
								environmentMappingEntity = static_cast<EnvironmentMapping*>(environmentMappingEntityCandidate);
							} else
							if (environmentMappingEntityCandidate->getEntityType() == Entity::ENTITYTYPE_ENTITYHIERARCHY) {
								auto entityHierarchyEnvironmentMappingEntity = static_cast<EntityHierarchy*>(environmentMappingEntityCandidate)->getEntityByType(Entity::ENTITYTYPE_ENVIRONMENTMAPPING);
								if (entityHierarchyEnvironmentMappingEntity != nullptr) environmentMappingEntity = static_cast<EnvironmentMapping*>(entityHierarchyEnvironmentMappingEntity);

							}
						}
						if (environmentMappingEntity != nullptr) {
							Vector3 environmentMappingTranslation;
							object->getTransformMatrix().getTranslation(environmentMappingTranslation);
							auto environmentMappingCubeMapTextureId = environmentMappingEntity->getCubeMapTextureId();
							Vector3 environmentMappingCubeMapPosition = object->hasReflectionEnvironmentMappingPosition() == true?object->getReflectionEnvironmentMappingPosition():environmentMappingTranslation;
							if (boundEnvironmentMappingCubeMapTextureId == -1) {
								boundEnvironmentMappingCubeMapTextureId = environmentMappingCubeMapTextureId;
								boundEnvironmentMappingCubeMapPosition = environmentMappingCubeMapPosition;
								renderer->setEnvironmentMappingCubeMapPosition(contextIdx, environmentMappingCubeMapPosition.getArray());
								renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_ENVIRONMENT);
								renderer->bindCubeMapTexture(contextIdx, environmentMappingCubeMapTextureId);
								renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
							} else
							if (boundEnvironmentMappingCubeMapTextureId != environmentMappingCubeMapTextureId || environmentMappingCubeMapPosition.equals(boundEnvironmentMappingCubeMapPosition) == false) {
								objectRenderContext.objectsNotRendered.push_back(object);
								continue;
							}
						}
					} else
					if (boundEnvironmentMappingCubeMapTextureId != -1) {
						objectRenderContext.objectsNotRendered.push_back(object);
						continue;
					}
					// set up effect color
					if ((renderTypes & RENDERTYPE_EFFECTCOLORS) == RENDERTYPE_EFFECTCOLORS) {
						fbEffectColorMuls.put(object->effectColorMul.getArray());
						fbEffectColorAdds.put(object->effectColorAdd.getArray());
					}

					// push mv, mvp to layouts
					fbMvMatrices.put(modelViewMatrix.getArray());
				}

				// it can happen that all faces to be rendered were transparent ones, check this and skip if feasible
				auto objectsToRenderIssue = fbMvMatrices.getPosition() / 16;
				if (objectsToRenderIssue > 0) {
					// upload model view matrices
					{
						renderer->uploadBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[0], fbMvMatrices.getPosition() * sizeof(float), &fbMvMatrices);
						renderer->bindModelMatricesBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[0]);
					}

					// upload effects
					if ((renderTypes & RENDERTYPE_EFFECTCOLORS) == RENDERTYPE_EFFECTCOLORS) {
						// upload effect color mul
						renderer->uploadBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[1], fbEffectColorMuls.getPosition() * sizeof(float), &fbEffectColorMuls);
						renderer->bindEffectColorMulsBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[1], 1);
						renderer->uploadBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[2], fbEffectColorAdds.getPosition() * sizeof(float), &fbEffectColorAdds);
						renderer->bindEffectColorAddsBufferObject(contextIdx, (*objectRenderContext.vboInstancedRenderingIds)[2], 1);
					}

					// set up texture matrix
					//	TODO: check if texture is in use
					if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES ||
						(renderTypes & RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY) == RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY) {
						renderer->getTextureMatrix(contextIdx).set(textureMatrix);
						renderer->onUpdateTextureMatrix(contextIdx);
					}

					// draw
					renderer->drawInstancedIndexedTrianglesFromBufferObjects(contextIdx, facesToRender, faceIdx, objectsToRenderIssue);
				}

				// clear list of objects we did not render
				objectRenderContext.objectsToRender = objectRenderContext.objectsNotRendered;
				objectRenderContext.objectsNotRendered.clear();

				// TODO: improve me!
				if (boundEnvironmentMappingCubeMapTextureId != -1) {
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_ENVIRONMENT);
					renderer->bindCubeMapTexture(contextIdx, renderer->ID_NONE);
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
				}
			} while (objectRenderContext.objectsToRender.size() > 0);

			// keep track of rendered faces
			faceIdx += faces;
		}
	}

	//
	if (cullingMode != 1) {
		renderer->enableCulling(contextIdx);
		cullingMode = 1;
	}

	// unbind buffers
	renderer->unbindBufferObjects(contextIdx);

	// reset objects to render
	objectRenderContext.objectsToRender.clear();
	objectRenderContext.objectsNotRendered.clear();
}

void EntityRenderer::setupMaterial(int contextIdx, ObjectNode* objectNode, int32_t facesEntityIdx, int32_t renderTypes, bool updateOnly, string& materialKey, const string& currentMaterialKey)
{
	auto& facesEntities = objectNode->node->getFacesEntities();
	auto material = facesEntities[facesEntityIdx].getMaterial();
	// get material or use default
	if (material == nullptr) material = Material::getDefaultMaterial();
	auto specularMaterialProperties = material->getSpecularMaterialProperties();
	auto pbrMaterialProperties = material->getPBRMaterialProperties();

	// material key
	materialKey = material->getId();

	// setup textures
	ObjectNode::setupTextures(renderer, contextIdx, objectNode, facesEntityIdx);

	//
	if (updateOnly == false) {
		if (renderer->getLighting(contextIdx) == renderer->LIGHTING_SPECULAR) {
			// apply materials
			if ((renderTypes & RENDERTYPE_MATERIALS) == RENDERTYPE_MATERIALS) {
				auto& rendererMaterial = renderer->getSpecularMaterial(contextIdx);
				rendererMaterial.ambient = specularMaterialProperties->getAmbientColor().getArray();
				rendererMaterial.diffuse = specularMaterialProperties->getDiffuseColor().getArray();
				rendererMaterial.specular = specularMaterialProperties->getSpecularColor().getArray();
				rendererMaterial.emission = specularMaterialProperties->getEmissionColor().getArray();
				rendererMaterial.shininess = specularMaterialProperties->getShininess();
				rendererMaterial.reflection = specularMaterialProperties->getReflection();
				rendererMaterial.diffuseTextureMaskedTransparency = specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true?1:0;
				rendererMaterial.diffuseTextureMaskedTransparencyThreshold = specularMaterialProperties->getDiffuseTextureMaskedTransparencyThreshold();
				renderer->onUpdateMaterial(contextIdx);
			}
			if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES) {
				// bind specular texture
				if (renderer->isSpecularMappingAvailable() == true) {
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_SPECULAR);
					renderer->bindTexture(contextIdx, objectNode->specularMaterialSpecularTextureIdsByEntities[facesEntityIdx]);
				}
				// bind normal texture
				if (renderer->isNormalMappingAvailable() == true) {
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_NORMAL);
					renderer->bindTexture(contextIdx, objectNode->specularMaterialNormalTextureIdsByEntities[facesEntityIdx]);
				}
				// switch back texture unit to diffuse unit
				renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
			}
		} else
		if (renderer->getLighting(contextIdx) == renderer->LIGHTING_PBR) {
			// apply materials
			if ((renderTypes & RENDERTYPE_MATERIALS) == RENDERTYPE_MATERIALS) {
				auto& rendererMaterial = renderer->getPBRMaterial(contextIdx);
				if (pbrMaterialProperties == nullptr) {
					rendererMaterial.baseColorFactor = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
					rendererMaterial.metallicFactor = 1.0f;
					rendererMaterial.roughnessFactor = 1.0f;
					rendererMaterial.normalScale = 1.0f;
					rendererMaterial.exposure = 1.0f;
					rendererMaterial.baseColorTextureMaskedTransparency = 0;
					rendererMaterial.baseColorTextureMaskedTransparencyThreshold = 0.0f;
				} else {
					rendererMaterial.baseColorFactor = pbrMaterialProperties->getBaseColorFactor().getArray();
					rendererMaterial.metallicFactor = pbrMaterialProperties->getMetallicFactor();
					rendererMaterial.roughnessFactor = pbrMaterialProperties->getRoughnessFactor();
					rendererMaterial.normalScale = pbrMaterialProperties->getNormalScale();
					rendererMaterial.exposure = pbrMaterialProperties->getExposure();
					rendererMaterial.baseColorTextureMaskedTransparency = pbrMaterialProperties->hasBaseColorTextureMaskedTransparency() == true?1:0;
					rendererMaterial.baseColorTextureMaskedTransparencyThreshold = pbrMaterialProperties->getBaseColorTextureMaskedTransparencyThreshold();
				}
				renderer->onUpdateMaterial(contextIdx);
			}
			if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES) {
				// bind metallic roughness texture
				renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_METALLICROUGHNESS);
				renderer->bindTexture(contextIdx, objectNode->pbrMaterialMetallicRoughnessTextureIdsByEntities[facesEntityIdx]);
				// bind normal texture
				renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_NORMAL);
				renderer->bindTexture(contextIdx, objectNode->pbrMaterialNormalTextureIdsByEntities[facesEntityIdx]);
				// switch back texture unit to base color unit
				renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_BASECOLOR);
			}
		}
	}

	// bind diffuse/base color texture
	if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES ||
		((renderTypes & RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY) == RENDERTYPE_TEXTURES_DIFFUSEMASKEDTRANSPARENCY)) {
		if (renderer->getLighting(contextIdx) == renderer->LIGHTING_SPECULAR) {
			auto& rendererMaterial = renderer->getSpecularMaterial(contextIdx);
			auto diffuseTexture = specularMaterialProperties->getDiffuseTexture();
			rendererMaterial.diffuseTextureMaskedTransparency = specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true?1:0;
			rendererMaterial.diffuseTextureMaskedTransparencyThreshold = specularMaterialProperties->getDiffuseTextureMaskedTransparencyThreshold();
			rendererMaterial.textureAtlasSize = specularMaterialProperties->getTextureAtlasSize();
			rendererMaterial.textureAtlasPixelDimension = { 0.0f, 0.0f };
			if (rendererMaterial.textureAtlasSize > 1 && diffuseTexture != nullptr) {
				rendererMaterial.textureAtlasPixelDimension[0] = 1.0f / diffuseTexture->getTextureWidth();
				rendererMaterial.textureAtlasPixelDimension[1] = 1.0f / diffuseTexture->getTextureHeight();
			}
			renderer->onUpdateMaterial(contextIdx);
			if ((renderTypes & RENDERTYPE_TEXTURES) == RENDERTYPE_TEXTURES ||
				specularMaterialProperties->hasDiffuseTextureMaskedTransparency() == true) {
				auto diffuseTextureId =
					objectNode->specularMaterialDynamicDiffuseTextureIdsByEntities[facesEntityIdx] != ObjectNode::TEXTUREID_NONE ?
					objectNode->specularMaterialDynamicDiffuseTextureIdsByEntities[facesEntityIdx] :
					objectNode->specularMaterialDiffuseTextureIdsByEntities[facesEntityIdx];
				materialKey+= ",";
				materialKey.append((const char*)&diffuseTextureId, sizeof(diffuseTextureId));
				if (updateOnly == false || currentMaterialKey.empty() == true) {
					renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
					renderer->bindTexture(contextIdx, diffuseTextureId);
				}
			}
		} else
		if (renderer->getLighting(contextIdx) == renderer->LIGHTING_PBR) {
			auto& rendererMaterial = renderer->getPBRMaterial(contextIdx);
			if (pbrMaterialProperties == nullptr) {
				rendererMaterial.baseColorTextureMaskedTransparency = 0;
				rendererMaterial.baseColorTextureMaskedTransparencyThreshold = 0.0f;
			} else {
				rendererMaterial.baseColorTextureMaskedTransparency = pbrMaterialProperties->hasBaseColorTextureMaskedTransparency() == true?1:0;
				rendererMaterial.baseColorTextureMaskedTransparencyThreshold = pbrMaterialProperties->getBaseColorTextureMaskedTransparencyThreshold();
			}
			renderer->onUpdateMaterial(contextIdx);
			renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_BASECOLOR);
			renderer->bindTexture(contextIdx, objectNode->pbrMaterialBaseColorTextureIdsByEntities[facesEntityIdx]);
		}
	}
}

void EntityRenderer::clearMaterial(int contextIdx)
{
	// TODO: optimize me! We do not need always to clear material
	if (renderer->getLighting(contextIdx) == renderer->LIGHTING_SPECULAR) {
		// unbind diffuse texture
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
		renderer->bindTexture(contextIdx, renderer->ID_NONE);
		// unbind specular texture
		if (renderer->isSpecularMappingAvailable() == true) {
			renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_SPECULAR);
			renderer->bindTexture(contextIdx, renderer->ID_NONE);
		}
		// unbind normal texture
		if (renderer->isNormalMappingAvailable()) {
			renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_NORMAL);
			renderer->bindTexture(contextIdx, renderer->ID_NONE);
		}
		// set diffuse texture unit
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::SPECULAR_TEXTUREUNIT_DIFFUSE);
	} else
	if (renderer->getLighting(contextIdx) == renderer->LIGHTING_PBR) {
		// unbind base color texture
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_BASECOLOR);
		renderer->bindTexture(contextIdx, renderer->ID_NONE);
		// unbind metallic roughness texture
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_METALLICROUGHNESS);
		renderer->bindTexture(contextIdx, renderer->ID_NONE);
		// unbind normal texture
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_NORMAL);
		renderer->bindTexture(contextIdx, renderer->ID_NONE);
		// set diffuse texture unit
		renderer->setTextureUnit(contextIdx, LightingShaderConstants::PBR_TEXTUREUNIT_BASECOLOR);
	}
}

void EntityRenderer::render(Entity::RenderPass renderPass, const vector<Entity*>& pses)
{
	// TODO: Move me into own class
	if (pses.size() == 0) return;

	//
	struct PseParameters {
		const Color4* effectColorAdd;
		const Color4* effectColorMul;
		int32_t textureHorizontalSprites;
		int32_t textureVerticalSprites;
		float pointSize;
		int32_t atlasTextureIndex;
	};
	unordered_map<void*, PseParameters> rendererPseParameters;

	// use default context
	auto contextIdx = renderer->CONTEXTINDEX_DEFAULT;

	// store model view matrix
	Matrix4x4 modelViewMatrix;
	modelViewMatrix.set(renderer->getModelViewMatrix());

	// set up renderer state
	renderer->enableBlending();
	// 	model view matrix
	renderer->getModelViewMatrix().identity();
	renderer->onUpdateModelViewMatrix(contextIdx);

	// switch back to texture unit 0, TODO: check where its set to another value but not set back
	renderer->setTextureUnit(contextIdx, 0);

	// merge pses, transform them into camera space and sort them
	auto& cameraMatrix = renderer->getCameraMatrix();
	for (auto entity: pses) {
		if (entity->getRenderPass() != renderPass) continue;
		auto ppse = dynamic_cast<PointsParticleSystem*>(entity);
		if (ppse != nullptr) {
			int atlasTextureIndex = engine->ppsTextureAtlas.getTextureIdx(ppse->getTexture());
			if (atlasTextureIndex == TextureAtlas::TEXTURE_IDX_NONE) continue;
			rendererPseParameters[ppse] = {
				.effectColorAdd = &ppse->getEffectColorAdd(),
				.effectColorMul = &ppse->getEffectColorMul(),
				.textureHorizontalSprites = ppse->getTextureHorizontalSprites(),
				.textureVerticalSprites = ppse->getTextureVerticalSprites(),
				.pointSize = ppse->getPointSize(),
				.atlasTextureIndex = atlasTextureIndex
			};
			renderTransparentRenderPointsPool->merge(ppse->getRenderPointsPool(), cameraMatrix);
		} else {
			auto fpse = dynamic_cast<FogParticleSystem*>(entity);
			if (fpse != nullptr) {
				int atlasTextureIndex = engine->ppsTextureAtlas.getTextureIdx(fpse->getTexture());
				if (atlasTextureIndex == TextureAtlas::TEXTURE_IDX_NONE) continue;
				rendererPseParameters[fpse] = {
					.effectColorAdd = &fpse->getEffectColorAdd(),
					.effectColorMul = &fpse->getEffectColorMul(),
					.textureHorizontalSprites = fpse->getTextureHorizontalSprites(),
					.textureVerticalSprites = fpse->getTextureVerticalSprites(),
					.pointSize = fpse->getPointSize(),
					.atlasTextureIndex = atlasTextureIndex
				};
				renderTransparentRenderPointsPool->merge(fpse->getRenderPointsPool(), cameraMatrix);
			}
		}
	}
	if (renderTransparentRenderPointsPool->getTransparentRenderPointsCount() > 0) {
		renderTransparentRenderPointsPool->sort();
		// render
		auto points =  renderTransparentRenderPointsPool->getTransparentRenderPoints();
		auto pseParameters = &rendererPseParameters.find(points[0]->particleSystem)->second;
		auto currentPpse = static_cast<void*>(points[0]->particleSystem);
		if (renderer->isSupportingIntegerProgramAttributes() == true) {
			for (auto i = 0; i < renderTransparentRenderPointsPool->getTransparentRenderPointsCount(); i++) {
				auto point = points[i];
				if (point->particleSystem != (void*)currentPpse) {
					pseParameters = &rendererPseParameters.find(point->particleSystem)->second;
					currentPpse = point->particleSystem;
				}
				psePointBatchRenderer->addPoint(
					point,
					pseParameters->atlasTextureIndex,
					pseParameters->pointSize,
					*pseParameters->effectColorMul,
					*pseParameters->effectColorAdd,
					pseParameters->textureHorizontalSprites,
					pseParameters->textureVerticalSprites
				);
			}
		} else {
			for (auto i = 0; i < renderTransparentRenderPointsPool->getTransparentRenderPointsCount(); i++) {
				auto point = points[i];
				if (point->particleSystem != (void*)currentPpse) {
					pseParameters = &rendererPseParameters.find(point->particleSystem)->second;
					currentPpse = point->particleSystem;
				}
				psePointBatchRenderer->addPointNoInteger(
					point,
					pseParameters->atlasTextureIndex,
					pseParameters->pointSize,
					*pseParameters->effectColorMul,
					*pseParameters->effectColorAdd,
					pseParameters->textureHorizontalSprites,
					pseParameters->textureVerticalSprites
				);
			}
		}

		// texture atlas
		engine->getParticlesShader()->setTextureAtlas(contextIdx, &engine->getPointParticleSystemTextureAtlas());

		// render, clear
		psePointBatchRenderer->render(contextIdx);
		psePointBatchRenderer->clear();

		// done
		renderTransparentRenderPointsPool->reset();
	}

	// unset renderer state
	renderer->disableBlending();
	// restore renderer state
	renderer->unbindBufferObjects(contextIdx);
	renderer->getModelViewMatrix().set(modelViewMatrix);
}

void EntityRenderer::render(Entity::RenderPass renderPass, const vector<Lines*>& linesEntities) {
	// TODO: Move me into own class
	// TODO: check me performance wise again
	if (linesEntities.size() == 0) return;

	// use default context
	auto contextIdx = renderer->CONTEXTINDEX_DEFAULT;

	// switch back to texture unit 0, TODO: check where its set to another value but not set back
	renderer->setTextureUnit(contextIdx, 0);

	// store model view matrix
	Matrix4x4 modelViewMatrix;
	modelViewMatrix.set(renderer->getModelViewMatrix());

	// set up renderer state
	renderer->enableBlending();

	//
	for (auto object: linesEntities) {
		if (object->getRenderPass() != renderPass) continue;

		// 	model view matrix
		renderer->getModelViewMatrix().set(object->getTransformMatrix()).multiply(renderer->getCameraMatrix());
		renderer->onUpdateModelViewMatrix(contextIdx);

		// render
		// issue rendering
		renderer->getEffectColorAdd(contextIdx) = object->getEffectColorAdd().getArray();
		renderer->getEffectColorMul(contextIdx) = object->getEffectColorMul().getArray();
		renderer->onUpdateEffect(contextIdx);

		// TODO: maybe use onBindTexture() or onUpdatePointSize()
		engine->getLinesShader()->setParameters(contextIdx, object->getTextureId(), object->getLineWidth());

		//
		renderer->bindVerticesBufferObject(contextIdx, (*object->vboIds)[0]);
		renderer->bindColorsBufferObject(contextIdx, (*object->vboIds)[1]);
		renderer->drawLinesFromBufferObjects(contextIdx, object->points.size(), 0);
	}

	// unbind texture
	renderer->bindTexture(contextIdx, renderer->ID_NONE);
	// unset renderer state
	renderer->disableBlending();
	// restore renderer state
	renderer->unbindBufferObjects(contextIdx);
	renderer->getModelViewMatrix().set(modelViewMatrix);
}
