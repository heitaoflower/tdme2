#pragma once

#include <string>

#include <tdme/tdme.h>
#include <tdme/engine/fwd-tdme.h>
#include <tdme/engine/subsystems/earlyzrejection/fwd-tdme.h>
#include <tdme/engine/subsystems/renderer/fwd-tdme.h>
#include <tdme/math/fwd-tdme.h>

using std::string;

using tdme::engine::subsystems::renderer::Renderer;
using tdme::engine::Engine;
using tdme::math::Matrix4x4;

/**
 * Early z rejection shader implementation
 * @author Andreas Drewke
 */
struct tdme::engine::subsystems::earlyzrejection::EZRShaderImplementation
{
	/**
	 * Destructor
	 */
	virtual ~EZRShaderImplementation() {}

	/**
	 * @return shader id
	 */
	virtual const string getId() = 0;

	/**
	 * @return if initialized and ready to use
	 */
	virtual bool isInitialized() = 0;

	/**
	 * Init EZR render shader program implementation
	 */
	virtual void initialize() = 0;

	/**
	 * Use EZR render shader program
	 * @param engine engine
	 * @param contextIdx context index
	 */
	virtual void useProgram(Engine* engine, int contextIdx) = 0;

	/**
	 * Unuse EZR render shader program
	 * @param contextIdx context index
	 */
	virtual void unUseProgram(int contextIdx) = 0;

	/**
	 * Update matrices
	 * @param renderer renderer
	 * @param contextIdx context index
	 */
	virtual void updateMatrices(Renderer* renderer, int contextIdx) = 0;

	/**
	 * update texture matrix
	 * @param renderer renderer
	 * @param contextIdx context index
	 */
	virtual void updateTextureMatrix(Renderer* renderer, int contextIdx) = 0;

	/**
	 * Update material
	 * @param renderer renderer
	 * @param contextIdx context index
	 */
	virtual void updateMaterial(Renderer* renderer, int contextIdx) = 0;

	/**
	 * Update shader parameters
	 * @param renderer renderer
	 * @param context
	 */
	virtual void updateShaderParameters(Renderer* renderer, int contextIdx) = 0;

	/**
	 * Bind texture
	 * @param renderer renderer
	 * @param contextIdx context index
	 * @param textureId texture id
	 */
	virtual void bindTexture(Renderer* renderer, int contextIdx, int32_t textureId) = 0;
};
