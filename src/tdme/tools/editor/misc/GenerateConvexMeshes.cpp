#include <tdme/tools/editor/misc/GenerateConvexMeshes.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <ext/v-hacd/src/VHACD_Lib/public/VHACD.h>

#include <tdme/tdme.h>
#include <tdme/engine/fileio/models/ModelReader.h>
#include <tdme/engine/fileio/models/TMWriter.h>
#include <tdme/engine/model/Color4.h>
#include <tdme/engine/model/Face.h>
#include <tdme/engine/model/FacesEntity.h>
#include <tdme/engine/model/Material.h>
#include <tdme/engine/model/Model.h>
#include <tdme/engine/model/Node.h>
#include <tdme/engine/model/RotationOrder.h>
#include <tdme/engine/model/SpecularMaterialProperties.h>
#include <tdme/engine/model/UpVector.h>
#include <tdme/engine/primitives/Triangle.h>
#include <tdme/engine/prototype/Prototype.h>
#include <tdme/engine/prototype/PrototypeBoundingVolume.h>
#include <tdme/engine/ObjectModel.h>
#include <tdme/gui/nodes/GUIElementNode.h>
#include <tdme/gui/nodes/GUINode.h>
#include <tdme/gui/nodes/GUINodeController.h>
#include <tdme/gui/nodes/GUIParentNode.h>
#include <tdme/gui/nodes/GUIScreenNode.h>
#include <tdme/os/filesystem/FileSystem.h>
#include <tdme/os/filesystem/StandardFileSystem.h>
#include <tdme/tools/editor/controllers/FileDialogScreenController.h>
#include <tdme/tools/editor/controllers/InfoDialogScreenController.h>
#include <tdme/tools/editor/controllers/ProgressBarScreenController.h>
#include <tdme/tools/editor/misc/PopUps.h>
#include <tdme/tools/editor/misc/Tools.h>
#include <tdme/utilities/Console.h>
#include <tdme/utilities/Exception.h>
#include <tdme/utilities/ExceptionBase.h>
#include <tdme/utilities/ModelTools.h>
#include <tdme/utilities/MutableString.h>

using tdme::tools::editor::misc::GenerateConvexMeshes;

using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

using namespace VHACD;

using tdme::engine::fileio::models::ModelReader;
using tdme::engine::fileio::models::TMWriter;
using tdme::engine::model::Color4;
using tdme::engine::model::Face;
using tdme::engine::model::FacesEntity;
using tdme::engine::model::Material;
using tdme::engine::model::Model;
using tdme::engine::model::Node;
using tdme::engine::model::RotationOrder;
using tdme::engine::model::SpecularMaterialProperties;
using tdme::engine::model::UpVector;
using tdme::engine::primitives::Triangle;
using tdme::engine::prototype::Prototype;
using tdme::engine::prototype::PrototypeBoundingVolume;
using tdme::engine::ObjectModel;
using tdme::gui::nodes::GUIElementNode;
using tdme::gui::nodes::GUINode;
using tdme::gui::nodes::GUINodeController;
using tdme::gui::nodes::GUIParentNode;
using tdme::gui::nodes::GUIScreenNode;
using tdme::os::filesystem::FileSystem;
using tdme::os::filesystem::StandardFileSystem;
using tdme::tools::editor::controllers::FileDialogScreenController;
using tdme::tools::editor::controllers::InfoDialogScreenController;
using tdme::tools::editor::controllers::ProgressBarScreenController;
using tdme::tools::editor::misc::PopUps;
using tdme::tools::editor::misc::Tools;
using tdme::utilities::Console;
using tdme::utilities::Exception;
using tdme::utilities::ExceptionBase;
using tdme::utilities::ModelTools;
using tdme::utilities::MutableString;

void GenerateConvexMeshes::removeConvexMeshes(Prototype* prototype)
{
	// delete old convex meshes
	for (int i = 0; i < prototype->getBoundingVolumeCount(); i++) {
		auto boundingVolume = prototype->getBoundingVolume(i);
		if (boundingVolume->isGenerated() == false) {
			continue;
		} else {
			if (boundingVolume->getConvexMeshFile().empty() == false &&
				FileSystem::getInstance()->fileExists(boundingVolume->getConvexMeshFile()) == true) {
				FileSystem::getInstance()->removeFile(
					FileSystem::getInstance()->getPathName(boundingVolume->getConvexMeshFile()),
					FileSystem::getInstance()->getFileName(boundingVolume->getConvexMeshFile())
				);
				prototype->removeBoundingVolume(i);
				i--;
			} else
			if (boundingVolume->getConvexMeshData().empty() == false) {
				prototype->removeBoundingVolume(i);
				i--;
			}
		}
	}
}

bool GenerateConvexMeshes::generateConvexMeshes(Prototype* prototype, Mode mode, PopUps* popUps, const string& pathName, const string& fileName, vector<vector<uint8_t>>& convexMeshTMsData, VHACD::IVHACD::Parameters parameters)
{
	auto success = true;
	if (mode == MODE_GENERATE) {
		class VHACDCallback : public IVHACD::IUserCallback {
			private:
				ProgressBarScreenController* progressBarScreenController;
			public:
				VHACDCallback(ProgressBarScreenController* progressBarScreenController): progressBarScreenController(progressBarScreenController) {}
				~VHACDCallback() {};
				void Update(
					const double overallProgress,
					const double stageProgress,
					const double operationProgress,
					const char* const stage,
					const char* const operation)
				{
					// progressBarScreenController->progress((int)(overallProgress + 0.5) / 100.0f);
				};
		};

		class VHACDLogger : public IVHACD::IUserLogger {
			public:
				VHACDLogger() {}
				~VHACDLogger() {};
				void Log(const char* const msg)
				{
					Console::println(msg);
				}
		};

		//
		// if (popUps != nullptr) popUps->getProgressBarScreenController()->show("Generate convex meshes ...");
		IVHACD* vhacd = CreateVHACD();
		try {
			if (parameters.m_resolution < 10000 || parameters.m_resolution > 64000000) {
				throw ExceptionBase("Resolution must be between 10000 and 64000000");
			}
			if (parameters.m_concavity < 0.0f || parameters.m_concavity > 1.0f) {
				throw ExceptionBase("Concavity must be between 0.0 and 1.0");
			}
			if (parameters.m_planeDownsampling < 1 || parameters.m_planeDownsampling > 16) {
				throw ExceptionBase("Plane down sampling must be between 1 and 16");
			}
			if (parameters.m_convexhullDownsampling < 1 || parameters.m_convexhullDownsampling > 16) {
				throw ExceptionBase("Convex hull down sampling must be between 1 and 16");
			}
			if (parameters.m_alpha < 0.0f || parameters.m_alpha > 1.0f) {
				throw ExceptionBase("Alpha must be between 0.0 and 1.0");
			}
			if (parameters.m_beta < 0.0f || parameters.m_beta > 1.0f) {
				throw ExceptionBase("Beta must be between 0.0 and 1.0");
			}
			if (parameters.m_maxNumVerticesPerCH < 4 || parameters.m_maxNumVerticesPerCH > 1024) {
				throw ExceptionBase("Max number of vertices per convex hull must be between 4 and 1024");
			}
			if (parameters.m_minVolumePerCH < 0.0f || parameters.m_minVolumePerCH > 0.01f) {
				throw ExceptionBase("Min volume per convex hull must be between 0.0 and 0.01");
			}
			if (parameters.m_pca > 1) {
				throw ExceptionBase("PCA must be between 0 and 1");
			}
			VHACDLogger vhacdLogger;
			parameters.m_logger = &vhacdLogger;
			/*
			if (popUps != nullptr) {
				VHACDCallback vhacdCallback(popUps->getProgressBarScreenController());
				parameters.m_callback = &vhacdCallback;
			}
			*/
			vector<float> meshPoints;
			vector<int> meshTriangles;
			auto meshModel = ModelReader::read(
				pathName,
				fileName
			);
			{
				ObjectModel meshObjectModel(meshModel);
				vector<Triangle> meshFaceTriangles;
				meshObjectModel.getTriangles(meshFaceTriangles);
				for (auto& triangle: meshFaceTriangles) {
					meshTriangles.push_back(meshPoints.size() / 3 + 0);
					meshTriangles.push_back(meshPoints.size() / 3 + 1);
					meshTriangles.push_back(meshPoints.size() / 3 + 2);
					for (auto i = 0; i < triangle.getVertices().size(); i++) {
						meshPoints.push_back(triangle.getVertices()[i].getX());
						meshPoints.push_back(triangle.getVertices()[i].getY());
						meshPoints.push_back(triangle.getVertices()[i].getZ());
					}
				}
			}
			delete meshModel;
			bool vhacdResult =
				vhacd->Compute(
					&meshPoints[0],
					(unsigned int)meshPoints.size() / 3,
					(const unsigned int *)&meshTriangles[0],
					(unsigned int)meshTriangles.size() / 3,
					parameters
				);
			if (vhacdResult == true) {
				auto convexHulls = vhacd->GetNConvexHulls();
				IVHACD::ConvexHull convexHull;
				for (auto i = 0; i < convexHulls; i++) {
					vhacd->GetConvexHull(i, convexHull);
					auto convexHullFileName = fileName + ".cm." + to_string(i) + ".tm";
					Console::println(
						"GenerateConvexMeshes::generateConvexMeshes(): VHACD: Saving convex hull@" +
						to_string(i) +
						": " + convexHullFileName +
						", points = " + to_string(convexHull.m_nPoints) +
						", triangles = " + to_string(convexHull.m_nTriangles)
					);
					auto convexHullModel = createModel(
						convexHullFileName,
						convexHull.m_points,
						convexHull.m_triangles,
						convexHull.m_nPoints,
						convexHull.m_nTriangles
					);
					convexMeshTMsData.push_back(vector<uint8_t>());
					TMWriter::write(convexHullModel, convexMeshTMsData[convexMeshTMsData.size() - 1]);
					delete convexHullModel;
				}
			}
		} catch (Exception &exception) {
			/*
			if (popUps != nullptr) {
				popUps->getInfoDialogScreenController()->show(
					"Warning: Could not create convex hulls",
					exception.what()
				);
			}
			*/
			Console::println(string("Could not create convex hulls: ") + exception.what());
			convexMeshTMsData.clear();
			success = false;
		}
		vhacd->Clean();
		vhacd->Release();
		// if (popUps != nullptr) popUps->getProgressBarScreenController()->close();
	} else
	if (mode == MODE_IMPORT) {
		try {
			auto meshModel = ModelReader::read(
				pathName,
				fileName
			);
			{
				ObjectModel meshObjectModel(meshModel);
				for (auto i = 0; i < meshObjectModel.getNodeCount(); i++) {
					vector<Triangle> nodeTriangles;
					meshObjectModel.getTriangles(nodeTriangles, i);
					auto convexHullFileName = fileName + ".cm." + to_string(i) + ".tm";
					Console::println(
						"GenerateConvexMeshes::generateConvexMeshes(): Model: Saving convex hull@" +
						to_string(i) +
						": " + convexHullFileName +
						", triangles = " + to_string(nodeTriangles.size())
					);
					auto convexHullModel = createModel(
						convexHullFileName,
						nodeTriangles
					);
					convexMeshTMsData.push_back(vector<uint8_t>());
					TMWriter::write(convexHullModel, convexMeshTMsData[convexMeshTMsData.size() - 1]);
					delete convexHullModel;
				}
			}
			delete meshModel;
		} catch (Exception &exception) {
			/*
			if (popUps != nullptr) {
				popUps->getInfoDialogScreenController()->show(
					"Warning: Could not create convex hulls",
					exception.what()
				);
			}
			*/
			Console::println(string("Could not create convex hulls: ") + exception.what());
			convexMeshTMsData.clear();
			success = false;
		}
	}
	return success;
}

Model* GenerateConvexMeshes::createModel(const string& id, double* points, unsigned int* triangles, unsigned int pointCount, unsigned int triangleCount) {
	auto model = new Model(id, id, UpVector::Y_UP, RotationOrder::XYZ, nullptr);
	auto material = new Material("primitive");
	material->setSpecularMaterialProperties(new SpecularMaterialProperties());
	material->getSpecularMaterialProperties()->setAmbientColor(Color4(0.5f, 0.5f, 0.5f, 1.0f));
	material->getSpecularMaterialProperties()->setDiffuseColor(Color4(1.0f, 0.5f, 0.5f, 0.5f));
	material->getSpecularMaterialProperties()->setSpecularColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));
	model->getMaterials()[material->getId()] = material;
	auto node = new Node(model, nullptr, "node", "node");
	vector<Vector3> vertices;
	vector<Vector3> normals;
	vector<Face> faces;
	int normalIndex = -1;
	for (auto i = 0; i < pointCount; i++) {
		vertices.push_back(
			Vector3(
				static_cast<float>(points[i * 3 + 0]),
				static_cast<float>(points[i * 3 + 1]),
				static_cast<float>(points[i * 3 + 2])
			)
		);
	}
	for (auto i = 0; i < triangleCount; i++) {
		normalIndex = normals.size();
		{
			array<Vector3, 3> faceVertices = {
				vertices[triangles[i * 3 + 0]],
				vertices[triangles[i * 3 + 1]],
				vertices[triangles[i * 3 + 2]]
			};
			for (auto& normal: ModelTools::computeNormals(faceVertices)) {
				normals.push_back(normal);
			}
		}
		faces.push_back(
			Face(
				node,
				triangles[i * 3 + 0],
				triangles[i * 3 + 1],
				triangles[i * 3 + 2],
				normalIndex + 0,
				normalIndex + 1,
				normalIndex + 2
			)
		);
	}
	FacesEntity nodeFacesEntity(node, "faces entity");
	nodeFacesEntity.setMaterial(material);
	nodeFacesEntity.setFaces(faces);
	vector<FacesEntity> nodeFacesEntities;
	nodeFacesEntities.push_back(nodeFacesEntity);
	node->setVertices(vertices);
	node->setNormals(normals);
	node->setFacesEntities(nodeFacesEntities);
	model->getNodes()["node"] = node;
	model->getSubNodes()["node"] = node;
	ModelTools::prepareForIndexedRendering(model);
	return model;
}

Model* GenerateConvexMeshes::createModel(const string& id, vector<Triangle>& triangles) {
	auto model = new Model(id, id, UpVector::Y_UP, RotationOrder::XYZ, nullptr);
	auto material = new Material("primitive");
	material->setSpecularMaterialProperties(new SpecularMaterialProperties());
	material->getSpecularMaterialProperties()->setAmbientColor(Color4(0.5f, 0.5f, 0.5f, 1.0f));
	material->getSpecularMaterialProperties()->setDiffuseColor(Color4(1.0f, 0.5f, 0.5f, 0.5f));
	material->getSpecularMaterialProperties()->setSpecularColor(Color4(0.0f, 0.0f, 0.0f, 1.0f));
	model->getMaterials()[material->getId()] = material;
	auto node = new Node(model, nullptr, "node", "node");
	vector<Vector3> vertices;
	vector<Vector3> normals;
	vector<Face> faces;
	auto index = 0;
	for (auto& triangle: triangles) {
		for (auto& vertex: triangle.getVertices()) {
			vertices.push_back(vertex);
		}
		{
			array<Vector3, 3> faceVertices = {
				triangle.getVertices()[0],
				triangle.getVertices()[1],
				triangle.getVertices()[2],
			};
			for (auto& normal: ModelTools::computeNormals(faceVertices)) {
				normals.push_back(normal);
			}
		}
		faces.push_back(
			Face(
				node,
				index + 0,
				index + 1,
				index + 2,
				index + 0,
				index + 1,
				index + 2
			)
		);
		index+= 3;
	}
	FacesEntity nodeFacesEntity(node, "faces entity");
	nodeFacesEntity.setMaterial(material);
	nodeFacesEntity.setFaces(faces);
	vector<FacesEntity> nodeFacesEntities;
	nodeFacesEntities.push_back(nodeFacesEntity);
	node->setVertices(vertices);
	node->setNormals(normals);
	node->setFacesEntities(nodeFacesEntities);
	model->getNodes()["node"] = node;
	model->getSubNodes()["node"] = node;
	ModelTools::prepareForIndexedRendering(model);
	return model;
}
