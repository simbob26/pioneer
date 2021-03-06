// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Ship.h"
#include "CargoBody.h"
#include "Game.h"
#include "Pi.h"
#include "Serializer.h"
#include "Sfx.h"
#include "Space.h"
#include "EnumStrings.h"
#include "LuaTable.h"
#include "collider/collider.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/ModelSkin.h"

void CargoBody::Save(Serializer::Writer &wr, Space *space)
{
	DynamicBody::Save(wr, space);
	m_cargo.Save(wr);
	wr.Float(m_hitpoints);
	wr.Float(m_selfdestructTimer);
	wr.Bool(m_hasSelfdestruct);
}

void CargoBody::Load(Serializer::Reader &rd, Space *space)
{
	DynamicBody::Load(rd, space);
	m_cargo.Load(rd);
	Init();
	m_hitpoints = rd.Float();
	m_selfdestructTimer = rd.Float();
	m_hasSelfdestruct = rd.Bool();
}

void CargoBody::Init()
{
	m_hitpoints = 1.0f;
	SetLabel(ScopedTable(m_cargo).CallMethod<std::string>("GetName"));
	SetMassDistributionFromModel();
	m_hasSelfdestruct = true;

	std::vector<Color> colors;
	//metallic blue-orangeish color scheme
	colors.push_back(Color(255, 198, 64));
	colors.push_back(Color(0, 222, 255));
	colors.push_back(Color(255, 255, 255));

	SceneGraph::ModelSkin skin;
	skin.SetColors(colors);
	skin.SetDecal("pioneer");
	skin.Apply(GetModel());
	GetModel()->SetColors(colors);

	Properties().Set("type", ScopedTable(m_cargo).CallMethod<std::string>("GetName"));
}

CargoBody::CargoBody(const LuaRef& cargo, float selfdestructTimer): m_cargo(cargo)
{
	SetModel("cargo");
	Init();
	SetMass(1.0);
	m_selfdestructTimer = selfdestructTimer; // number of seconds to live

	if (is_zero_exact(selfdestructTimer)) // turn off self destruct
		m_hasSelfdestruct = false;
}

void CargoBody::TimeStepUpdate(const float timeStep)
{

	// Suggestion: since cargo doesn't need thrust or AI, it could be
	// converted into an idle object on orbital rails, set up to only take
	// memory & save file space (not CPU power) when far from the player.
	// Until then, we kill it after some time, to not clutter up the current
	// star system.

	if (m_hasSelfdestruct) {
		m_selfdestructTimer -= timeStep;
		if (m_selfdestructTimer <= 0){
			Pi::game->GetSpace()->KillBody(this);
			Sfx::Add(this, Sfx::TYPE_EXPLOSION);
		}
	}
	DynamicBody::TimeStepUpdate(timeStep);
}

bool CargoBody::OnDamage(Object *attacker, float kgDamage, const CollisionContact& contactData)
{
	m_hitpoints -= kgDamage*0.001f;
	if (m_hitpoints < 0) {
		Pi::game->GetSpace()->KillBody(this);
		Sfx::Add(this, Sfx::TYPE_EXPLOSION);
	}
	return true;
}

bool CargoBody::OnCollision(Object *b, Uint32 flags, double relVel)
{
	// ignore collision if its about to be scooped
	if (b->IsType(Object::SHIP)) {
		int cargoscoop_cap = 0;
		static_cast<Ship*>(b)->Properties().Get("cargo_scoop_cap", cargoscoop_cap);
		if (cargoscoop_cap > 0)
			return true;
	}

	return DynamicBody::OnCollision(b, flags, relVel);
}

void CargoBody::Render(Graphics::Renderer *r, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform)
{
	RenderModel(r, camera, viewCoords, viewTransform);
}

void CargoBody::SetLabel(const std::string &label)
{
	assert(GetModel());
	GetModel()->SetLabel(label);
	Body::SetLabel(label);
}
