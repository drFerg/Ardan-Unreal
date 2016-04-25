// Author: Fergus Leahy

#include "Virtual_CPS_World.h"
#include "Node.h"
#include <vector>

#define LEDON 500
#define LEDOFF 0

//ATriggerBox* tb;

Node::Node(AActor* sensor)
{
	UE_LOG(LogNet, Log, TEXT("New Sensor: %s"), *(sensor->GetName()))
	actor = sensor;
	actor->GetComponents(Leds);

	for (USpotLightComponent *l: Leds)
	{
		if (l == NULL) continue;
		UE_LOG(LogNet, Log, TEXT("%s"), *(l->GetName()));
		UE_LOG(LogNet, Log, TEXT("Owner: %s"), *(l->GetOwner()->GetName()));
		l->SetIntensity(LEDOFF);
	}
	TArray<AActor*> attachedActors;
	//tb = actor->GetComponentByClass(ATriggerBox);
	actor->GetAttachedActors(attachedActors);
	for (AActor *a: attachedActors) {
		UE_LOG(LogNet, Log, TEXT("%s"), *(a->GetName()));
//		if (a->GetClass() == ATriggerBox::StaticClass()){
//			UE_LOG(LogNet, Log, TEXT("Found: %s"), *(a->GetName()));
//			tb = a;
//			tb->on
//			tb->OnActorBeginOverlap.AddDynamic(sensor, Node::OnBeginTriggerOverlap);
//		}
	}

}

void Node::OnBeginTriggerOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {

}

void Node::Led(int32 led, bool on)
{
	if (led > 3 && led < 0) return;
	// UE_LOG(LogNet, Log, TEXT("Node: %s"), *(actor->GetName()))
	// UE_LOG(LogNet, Log, TEXT("Led: %s (%i)"), *(Leds[led]->GetName()), led);
	Leds[led]->SetIntensity(on ? LEDON : LEDOFF);
}

void Node::SetLed(uint8 R, uint8 G, uint8 B)
{
	//UE_LOG(LogNet, Log, TEXT("Node: %s"), *(actor->GetName()))
	Leds[0]->SetIntensity(R ? LEDON : LEDOFF);
	Leds[1]->SetIntensity(G ? LEDON : LEDOFF);
	Leds[2]->SetIntensity(B ? LEDON : LEDOFF);
}
Node::~Node()
{
}
