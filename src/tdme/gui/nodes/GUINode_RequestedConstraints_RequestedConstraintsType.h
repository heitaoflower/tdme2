#pragma once

#include <string>

#include <tdme/tdme.h>
#include <tdme/gui/nodes/fwd-tdme.h>
#include <tdme/utilities/Enum.h>

using std::string;

using tdme::gui::nodes::GUINode;
using tdme::gui::nodes::GUINode_Alignments;
using tdme::gui::nodes::GUINode_AlignmentHorizontal;
using tdme::gui::nodes::GUINode_AlignmentVertical;
using tdme::gui::nodes::GUINode_Border;
using tdme::gui::nodes::GUINode_ComputedConstraints;
using tdme::gui::nodes::GUINode_Flow;
using tdme::gui::nodes::GUINode_Padding;
using tdme::gui::nodes::GUINode_RequestedConstraints;
using tdme::gui::nodes::GUINode_RequestedConstraints_RequestedConstraintsType;
using tdme::utilities::Enum;

/**
 * GUI node requested constraints requested constraints type enum
 * @author Andreas Drewke
 */
class tdme::gui::nodes::GUINode_RequestedConstraints_RequestedConstraintsType final: public Enum
{
public:
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* NONE;
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* PIXEL;
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* PERCENT;
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* STAR;
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* AUTO;
	STATIC_DLL_IMPEXT static GUINode_RequestedConstraints_RequestedConstraintsType* TABLECELL;

	/**
	 * Public constructor
	 * @param name name
	 * @param ordinal ordinal
	 */
	GUINode_RequestedConstraints_RequestedConstraintsType(const string& name, int ordinal);

	/**
	 * Returns enum object given by name
	 * @param name name
	 * @return enum object
	 */
	static GUINode_RequestedConstraints_RequestedConstraintsType* valueOf(const string& name);

};
