#include <tdme/gui/nodes/GUINodeConditions.h>

#include <vector>

#include <tdme/tdme.h>
#include <tdme/gui/nodes/GUIElementNode.h>
#include <tdme/gui/nodes/GUINode_RequestedConstraints_RequestedConstraintsType.h>
#include <tdme/gui/nodes/GUIScreenNode.h>

using std::vector;

using tdme::gui::nodes::GUIElementNode;
using tdme::gui::nodes::GUINode_RequestedConstraints_RequestedConstraintsType;
using tdme::gui::nodes::GUINodeConditions;
using tdme::gui::nodes::GUIScreenNode;

GUINodeConditions::GUINodeConditions(GUIElementNode* elementNode): elementNode(elementNode)
{
}

void GUINodeConditions::updateNode(GUINode* node, const vector<string>& conditions) const {
	node->conditionsMet = node->checkConditions();
	node->onSetConditions(conditions);
	auto parentNode = dynamic_cast<GUIParentNode*>(node);
	if (parentNode != nullptr) {
		for (auto i = 0; i < parentNode->subNodes.size(); i++) {
			auto guiSubNode = parentNode->subNodes[i];
			updateNode(guiSubNode, conditions);
		}
	}
}

void GUINodeConditions::updateElementNode(const vector<string>& conditions) const {
	if (elementNode == nullptr) return;

	// update related element node
	elementNode->onSetConditions(conditions);
	for (auto i = 0; i < elementNode->subNodes.size(); i++) {
		auto guiSubNode = elementNode->subNodes[i];
		updateNode(guiSubNode, conditions);
	}
	auto screenNode = elementNode->getScreenNode();
	screenNode->invalidateLayout(elementNode);

	// also update nodes that belong to this conditions but are not with in sub tree of related element node
	auto elementNodeToNodeMappingIt = screenNode->elementNodeToNodeMapping.find(elementNode->getId());
	if (elementNodeToNodeMappingIt != screenNode->elementNodeToNodeMapping.end()) {
		for (auto& nodeId: elementNodeToNodeMappingIt->second) {
			auto node = screenNode->getNodeById(nodeId);
			if (node == nullptr) continue;
			updateNode(node, conditions);
			screenNode->invalidateLayout(node);
		}
	}
}
