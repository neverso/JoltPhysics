// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <TestFramework.h>

#include <Tests/General/ContactListenerTest.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/EstimateCollisionResponse.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Layers.h>
#include <Renderer/DebugRendererImp.h>

JPH_IMPLEMENT_RTTI_VIRTUAL(ContactListenerTest) 
{ 
	JPH_ADD_BASE_CLASS(ContactListenerTest, Test) 
}

void ContactListenerTest::Initialize()
{
	// Floor
	CreateFloor();

	RefConst<Shape> box_shape = new BoxShape(Vec3(0.5f, 1.0f, 2.0f));

	// Dynamic body 1
	Body &body1 = *mBodyInterface->CreateBody(BodyCreationSettings(box_shape, RVec3(0, 10, 0), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING));
	body1.SetAllowSleeping(false);
	mBodyInterface->AddBody(body1.GetID(), EActivation::Activate);

	// Dynamic body 2
	Body &body2 = *mBodyInterface->CreateBody(BodyCreationSettings(box_shape, RVec3(5, 10, 0), Quat::sRotation(Vec3::sAxisX(), 0.25f * JPH_PI), EMotionType::Dynamic, Layers::MOVING));
	body2.SetAllowSleeping(false);
	mBodyInterface->AddBody(body2.GetID(), EActivation::Activate);

	// Dynamic body 3
	Body &body3 = *mBodyInterface->CreateBody(BodyCreationSettings(new SphereShape(2.0f), RVec3(10, 10, 0), Quat::sRotation(Vec3::sAxisX(), 0.25f * JPH_PI), EMotionType::Dynamic, Layers::MOVING));
	body3.SetAllowSleeping(false);
	mBodyInterface->AddBody(body3.GetID(), EActivation::Activate);

	// Dynamic body 4
	Ref<StaticCompoundShapeSettings> compound_shape = new StaticCompoundShapeSettings;
	compound_shape->AddShape(Vec3::sZero(), Quat::sIdentity(), new CapsuleShape(5, 1));
	compound_shape->AddShape(Vec3(0, -5, 0), Quat::sIdentity(), new SphereShape(2));
	compound_shape->AddShape(Vec3(0, 5, 0), Quat::sIdentity(), new SphereShape(2));
	Body &body4 = *mBodyInterface->CreateBody(BodyCreationSettings(compound_shape, RVec3(15, 10, 0), Quat::sRotation(Vec3::sAxisX(), 0.25f * JPH_PI), EMotionType::Dynamic, Layers::MOVING));
	body4.SetAllowSleeping(false);
	mBodyInterface->AddBody(body4.GetID(), EActivation::Activate);
	
	// Store bodies for later use
	mBody[0] = &body1;
	mBody[1] = &body2;
	mBody[2] = &body3;
	mBody[3] = &body4;
}

void ContactListenerTest::PostPhysicsUpdate(float inDeltaTime)
{
	for (Body *body : mBody)
		Trace("State, body: %08x, v=%s, w=%s", body->GetID().GetIndex(), ConvertToString(body->GetLinearVelocity()).c_str(), ConvertToString(body->GetAngularVelocity()).c_str());
}

ValidateResult ContactListenerTest::OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult)
{
	// Body 1 and 2 should never collide
	return ((&inBody1 == mBody[0] && &inBody2 == mBody[1]) || (&inBody1 == mBody[1] && &inBody2 == mBody[0]))? ValidateResult::RejectAllContactsForThisBodyPair : ValidateResult::AcceptAllContactsForThisBodyPair;
}

void ContactListenerTest::OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings)
{
	// Make body 1 bounce only when a new contact point is added but not when it is persisted (its restitution is normally 0)
	if (&inBody1 == mBody[0] || &inBody2 == mBody[0])
	{
		JPH_ASSERT(ioSettings.mCombinedRestitution == 0.0f);
		ioSettings.mCombinedRestitution = 1.0f;
	}

	// Estimate the contact impulses. Note that these won't be 100% accurate unless you set the friction of the bodies to 0 (EstimateCollisionResponse ignores friction)
	ContactImpulses impulses;
	Vec3 v1, w1, v2, w2;
	EstimateCollisionResponse(inBody1, inBody2, inManifold, v1, w1, v2, w2, impulses, ioSettings.mCombinedRestitution);

	// Trace the result
	String impulses_str;
	for (float impulse : impulses)
		impulses_str += StringFormat("%f ", (double)impulse);

	Trace("Estimated velocity after collision, body1: %08x, v=%s, w=%s, body2: %08x, v=%s, w=%s, impulses: %s",
		inBody1.GetID().GetIndex(), ConvertToString(v1).c_str(), ConvertToString(w1).c_str(),
		inBody2.GetID().GetIndex(), ConvertToString(v2).c_str(), ConvertToString(w2).c_str(),
		impulses_str.c_str());
}
