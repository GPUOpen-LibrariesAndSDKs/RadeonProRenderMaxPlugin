/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* The fundamental scene parsing implementation
*********************************************************************************************************************************/
#pragma once
#include "frWrap.h"

#include "Common.h"
#include <RadeonProRender.h>
#include "MaterialParser.h"
#include <vector>
#include <string>
#include <FrScope.h>

FIRERENDER_NAMESPACE_BEGIN;
struct RenderParameters;
class SceneCallbacks;

void SetNameFromNode(INode* node, frw::Object& obj);

/// Types of camera projection we recognize
enum ProjectionType {
	P_PERSPECTIVE,
	P_ORTHO,
	P_CYLINDRICAL,
};

/// A common structure with all necessary data parsed from any type of view data we get
struct ParsedView {

	/// Camera transform matrix
	Matrix3 tm;
	Matrix3 tmMotion;

	/// Horizontal FOV, in radians
	float perspectiveFov;

	ProjectionType projection;
	int projectionOverride;

	/// Width of the ortho projection sensor, in world units
	float orthoSize;

	// DOF parameters

	bool useDof;

	float fSTop;

	/// Number of aperture blades used when simulating bokeh effect
	int bokehBlades;

	/// Distance to focus plane, in world units
	float focusDistance;

	/// Width of the camera sensor, in world units
	float sensorWidth;

	/// Focal Length
	float focalLength;

	/// Perspective correction
	Point2 tilt;

	bool useMotionBlur;
	float cameraExposure;
	float motionBlurScale;

	std::string cameraNodeName;

	bool isSame(const ParsedView& other, bool *needResetScenePtr) const {
		bool result = useDof == other.useDof && bokehBlades == other.bokehBlades && orthoSize == other.orthoSize &&
			sensorWidth == other.sensorWidth && fSTop == other.fSTop && perspectiveFov == other.perspectiveFov &&
			projection == other.projection && focusDistance == other.focusDistance && tm == other.tm &&
			projectionOverride == other.projectionOverride &&
			useMotionBlur == other.useMotionBlur && cameraExposure == other.cameraExposure && motionBlurScale== other.motionBlurScale
			;
		bool needResetScene = false;
		if(!result){
			needResetScene  = (useMotionBlur!=other.useMotionBlur) ||
				(useMotionBlur && (motionBlurScale != other.motionBlurScale));
		}
		if(needResetScenePtr)
			*needResetScenePtr = needResetScene;
		return result;
	}
};

/// INode waiting to be parsed. It can have optionally an override transform matrix specified (which we need so we can parse 
/// scatter systems that provide us with single INode and list of its copies with different transforms)
class ParsedNode
{
public:
	INode*  node = nullptr;
	Mtl* material = nullptr;
	Object* object = nullptr;

	// kInvalidAnimHandle is zero, why we are using -1???
	static const size_t INVALID_ID = (size_t)-1;

	static bool isValidId(size_t id){
		return id!=INVALID_ID;
	}

	size_t	id = INVALID_ID;		// this will generally be derived from AnimHandle
	size_t	matId = INVALID_ID;  // ditto
	size_t  objectId = INVALID_ID;

	DWORD invalidationTimestamp = 0;

	Matrix3 tm;
	Matrix3 tmMotion;

	ParsedNode() { tm.IdentityMatrix();	}

	ParsedNode(size_t id, INode* node, const Matrix3& tm);

	std::vector<Mtl*> GetAllMaterials(const TimeValue &t) const;
};

class ParsedMap
{
public:
	Texmap* texmap = nullptr;
	size_t id = 0;
};

/// The fundamental class which parses the whole scene into Radeon ProRender format. The sequence of calls to parse everything is:
/// parseView()
/// parseScene()
/// if running material editor preview: parseDefaultLights()
/// parseEnvironment()
class SceneParser {
	SceneParser(const SceneParser&) = delete;
	SceneParser& operator=(const SceneParser&) = delete;
public:
	SceneParser(RenderParameters& params, SceneCallbacks& callbacks, frw::Scope scope);

	typedef std::list<ParsedNode> ParsedNodes;

	bool hasDirectDisplacements;

	struct ParsedEnvironment
	{
		BOOL enabled;
		Matrix3 tm;
		Texmap* envMap;
		Texmap* envReflMap;
		Texmap* envRefrMap;
		Color envColor;
		float intensity;
		float saturation;
		float backgroundIntensity;
		Texmap* backgroundMap;

		bool sky; // use sun-sky if true, otherwise use IBL

		std::vector<frw::Shape> portals;

		ParsedEnvironment()
		{
			enabled = false;
			envMap = 0;
			envReflMap = 0;
			envRefrMap = 0;
			envColor = Color(0, 0, 0);
			intensity = 1.f;
			saturation = 0.5f;
			backgroundIntensity = 1.f;;
			backgroundMap = 0;
			sky = false;
			portals.clear();
		}
	};

	struct ParsedGround
	{
		BOOL enabled;
		Matrix3 tm;
		float radius;
		float groundHeight;
		BOOL hasShadows;
		//float shadowsDensity; What is this?
		BOOL hasReflections;
		float reflectionsRoughness;
		Color reflectionsColor;

		ParsedGround()
		{
			enabled = false;
			radius = 0.f;
			groundHeight = 0.f;
			hasShadows = false;
			//shadowsDensity = 0.f; What is this?
			hasReflections = 0;
			reflectionsRoughness = 0.f;
			reflectionsColor = Color(0, 0, 0);
		}
	};

	struct ParsedData
	{
		TimeValue t;
		ParsedNodes nodes;
		ParsedMap	environment;
		ParsedEnvironment frEnvironment;
		ParsedGround frGround;
		int defaultLightCount = 0;
	} parsed;

	struct Flags
	{
		bool usesNonPMLights = false;	// set when nodes are parsed
	} flags;

protected:


		// gather misc data here for a render rather than accessing it all over the place
		// easier to detect changes this way
	struct MaxSceneState
	{
		float timeValue = 0;
		Color bgColor;
		// environment
		int envotype;
		Color envoColor;
		BOOL useIBLMap = FALSE;
		BOOL useBackplate = FALSE;
		BOOL useReflectionMap = FALSE;
		BOOL useRefractionMap = FALSE;
		BOOL useSkyReflectionMap = FALSE;
		BOOL useSkyRefractionMap = FALSE;
		BOOL useSkyBackplate = FALSE;
		float intensity;
		// sky
		int skyType;
		float skyAzimuth;
		float skyAltitude;
		int skyHours;
		int skyMinutes;
		int skySeconds;
		int skyDay;
		int skyMonth;
		int skyYear;
		int skyTimezone;
		float skyLatitude;
		float skyLongitude;
		BOOL skyDaysaving;
		float skyHaze;
		Color skyGroundColor;
		Color skyGroundAlbedo;
		Color skyFilterColor;
		float skySunDiscSize;
		float skyIntensity;
		float skySaturation;

		// ground
		BOOL groundUse;
		float groundRadius;
		float groundHeight;
		BOOL groundShadows;
		float groundReflStrength;
		BOOL groundRefl;
		Color groundReflColor;
		float groundReflRough;
	} sceneState;
	
	float masterScale; // positional units conversion to meters

	frw::Scope		scope;	// this scope is persistent with the scene
	frw::Scene		scene;

	/// Storage of all render parameters and settings
	RenderParameters& params;

	/// Object calling all required callbacks on 3ds Max nodes
	SceneCallbacks& callbacks;

	/// Handle to the currently used environment light (set up by parseEnvironment()).
	frw::EnvironmentLight enviroLight;

	/// Handle to the currently used environment reflection override light
	frw::EnvironmentLight enviroReflLight;

	/// Handle to the currently used environment refraction override light
	frw::EnvironmentLight enviroRefrLight;

	/// Parses single 3ds Max light (nonphysical, non-area) and outputs it in provided scene
	/// \param node Scene node holding the light
	/// \param evaluatedObject result of calling EvalWorldState on node
	void parseMaxLight(const ParsedNode& parsedNode, Object* evaluatedObject);

	/// Parses our own portal light
	/// \param node Scene node holding the light
	/// \param evaluatedObject result of calling EvalWorldState on node
	void parseFRPortal(const ParsedNode& parsedNode, Object* evaluatedObject);

	/// Parses Corona sun object and approximates it by a directional light in provided scene
	/// \param node Scene node holding the light
	/// \param evaluatedObject result of calling EvalWorldState on node
	void parseCoronaSun(const ParsedNode& parsedNode, Object* object);

	std::vector<frw::Shape> parseMesh(INode* inode, Object* evaluatedObject, const int numSubmtls, size_t& meshFaces, bool flipFaces);
	// track nodes so we can later destroy them

	void traverseNode(INode* input, const bool processXRef, const RenderParameters& parameters, ParsedNodes& output);

	void traverseMaterialUpdate(Mtl* material);

public:
	MaterialParser mtlParser;
	ParsedView view;

	DWORD syncTimestamp = 0;
public:

	/// Parses the active view based on current render settings and outputs it in provided object
	ParsedView ParseView();

	void ParseScene();

	/// Parses all scene objects and outputs them in provided scene
	void AddParsedNodes(const ParsedNodes& parsedNodes);

	HashValue GetMaxSceneHash();

	bool NeedsUpdate();

	bool SynchronizeView(bool forceUpdate);

	/// Make sure FR state reflects Max state, returns true if scene was changed
	bool Synchronize(bool forceUpdate);

	/// Used by material editor rendering, only interested in the one object's material (material preview)
	Mtl* parseMaterialPreview(frw::Shader& surfaceShader, frw::Shader& volumeShader, bool &castShadows);

	/// Parses a list of default lights (which are a different object from standard 3ds max light shaders) and outputs them to a 
	/// scene
	void parseDefaultLights(const Stack<DefaultLight>& defaultLights);

	/// Parses the scene environment light and stores it in the scene. Must be run BEFORE parseView, because it sets up the enviro 
	/// light which is needed later for portals. Returns true is an environment was found. false otherwise.
	bool parseEnvironment(Texmap* enviroMap);

	void useFREnvironment();
	bool parseFREnvironment(const ParsedNode& parsedNode, Object* object);

	void useFRGround();
	bool parseFRGround();
};

// Scene Attach Callback, handles additional operations when needed for frw::Scene::Attach()
// Used for ActiveShade during scene export (translation)
class SceneAttachCallback
{
	public:
		virtual void PreSceneAttachLight( frw::Shape& shape, INode* node )=0;
		virtual void PreSceneAttachLight( frw::Light& light, INode* node )=0;
};

FIRERENDER_NAMESPACE_END;
