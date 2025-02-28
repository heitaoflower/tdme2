#pragma once

#include <string>
#include <vector>

#include <tdme/tdme.h>
#include <tdme/gui/events/fwd-tdme.h>
#include <tdme/gui/events/GUIActionListener.h>
#include <tdme/gui/events/GUIChangeListener.h>
#include <tdme/gui/events/GUIContextMenuRequestListener.h>
#include <tdme/gui/events/GUIFocusListener.h>
#include <tdme/gui/events/GUITooltipRequestListener.h>
#include <tdme/gui/nodes/fwd-tdme.h>
#include <tdme/tools/editor/misc/fwd-tdme.h>
#include <tdme/tools/editor/tabcontrollers/TabController.h>
#include <tdme/tools/editor/tabviews/fwd-tdme.h>
#include <tdme/utilities/fwd-tdme.h>
#include <tdme/utilities/MiniScript.h>

#include <ext/tinyxml/tinyxml.h>

using std::string;
using std::vector;

using tdme::gui::events::GUIActionListener;
using tdme::gui::events::GUIActionListenerType;
using tdme::gui::events::GUIChangeListener;
using tdme::gui::events::GUIContextMenuRequestListener;
using tdme::gui::events::GUIFocusListener;
using tdme::gui::events::GUITooltipRequestListener;
using tdme::gui::nodes::GUIElementNode;
using tdme::gui::nodes::GUINode;
using tdme::gui::nodes::GUIParentNode;
using tdme::gui::nodes::GUIScreenNode;
using tdme::gui::nodes::GUITextNode;
using tdme::tools::editor::misc::PopUps;
using tdme::tools::editor::tabcontrollers::TabController;
using tdme::tools::editor::tabviews::TextEditorTabView;
using tdme::utilities::MiniScript;

using tinyxml::TiXmlAttribute;
using tinyxml::TiXmlDocument;
using tinyxml::TiXmlElement;

/**
 * Text editor tab controller
 * @author Andreas Drewke
 */
class tdme::tools::editor::tabcontrollers::TextEditorTabController final
	: public TabController
{
public:
	struct MiniScriptScriptSyntaxTree {
		MiniScript::Script::ScriptType type;
		string condition;
		string name;
		MiniScript::ScriptSyntaxTreeNode conditionSyntaxTree;
		vector<MiniScript::ScriptSyntaxTreeNode> syntaxTree;
	};

private:
	TextEditorTabView* view { nullptr };
	GUIScreenNode* screenNode { nullptr };
	PopUps* popUps { nullptr };
	vector<MiniScriptScriptSyntaxTree> miniScriptSyntaxTrees;
	bool firstSearch { true };
	int searchIndex { -1 };

public:
	/**
	 * Public constructor
	 * @param view view
	 */
	TextEditorTabController(TextEditorTabView* view);

	/**
	 * Destructor
	 */
	virtual ~TextEditorTabController();

	/**
	 * Get view
	 */
	TextEditorTabView* getView();

	// overridden method
	GUIScreenNode* getScreenNode() override;

	// overridden methods
	void initialize(GUIScreenNode* screenNode) override;
	void dispose() override;
	void onChange(GUIElementNode* node) override;
	void onAction(GUIActionListenerType type, GUIElementNode* node) override;
	void onFocus(GUIElementNode* node) override;
	void onUnfocus(GUIElementNode* node) override;
	void onContextMenuRequest(GUIElementNode* node, int mouseX, int mouseY) override;
	void onTooltipShowRequest(GUINode* node, int mouseX, int mouseY) override;
	void onTooltipCloseRequest() override;
	void onCommand(TabControllerCommand command) override;
	void onDrop(const string& payload, int mouseX, int mouseY) override;

	/**
	 * Set outliner content
	 */
	void setOutlinerContent();

	/**
	 * Set outliner add drop down content
	 */
	void setOutlinerAddDropDownContent();

	/**
	 * @return miniscript syntax trees
	 */
	inline const vector<MiniScriptScriptSyntaxTree>& getMiniScriptSyntaxTrees() {
		return miniScriptSyntaxTrees;
	}

	/**
	 * Update MiniScript syntax tree
	 * @param miniScriptScriptIdx MiniScript script index
	 */
	void updateMiniScriptSyntaxTree(int miniScriptScriptIdx);

	/**
	 * Show the information pop up / modal
	 * @param caption caption
	 * @param message message
	 */
	void showInfoPopUp(const string& caption, const string& message);

	/**
	 * Close find/replace window
	 */
	void closeFindReplaceWindow();

};
