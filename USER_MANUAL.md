# SpatialOS GDK for Unreal Test Gyms Project 

#### What is a gym
* A gym is a level containing a minimal number of Actors and simple behaviors; it provides the minimum game content necessary to demonstrate a piece of the SpatialOS GDK for Unreal's functionality.
* We use gyms to help quickly visualize if a feature is working.

#### Project layout
```
Content
|
|__Actors
|
|__Characters
|
|__GASComponents
|
|__GameModes
|
|__Maps
|
|__Meshes
|
|__Spatial
```

Gym levels are in `Content\Maps`: add any new gyms in this directory.
Actors used in gyms are in `Contetnt\Actors`: add any new Actors to this directory.

#### How to add a gym 
1. In the Test Gyms Project, create a new level, with a descriptive name, and store it in the `Content\Maps` directory;
  * consider copying the `EmptyGym` level in `Content\Maps` as a starting point.
2. Consider adding a text object containing the new gym name to the new level you create;
  * this aids identification. (For example, you can see which gym a screenshot is from.)
3. Populate the new level with the Actors required to demonstrate the functionality of this gym.
  * Use existing Actors in the Test Gyms Project if possible, in order to keep it minimal.
  * If you create new Actors, store them in the `Content\Actors` directory.
4. Add a description of the new gym level to this `USER_MANUAL.md` document, in the section below;
  * breifly describe what it demonstrates and how to use it. 

#### Current gyms

##### Empty gym
* The template for creating new gyms. Copy this to use as a starting point for your own gym.

##### Handover gym
* Demonstrates that an entity can cross from one area of authority to another.
* Contains a cube that moves back and forth across a floor.

##### FASHandover gym
* Fast Array Serialization handover gym.
* Demonstrates that an actor with an ability system component can transition between workers correctly.
* Internally GAS uses Fast Array Serialization.
* Validation:
  1. A new GameplayEffect is added by the authoritative server on authority gained.
  2. Additionally a handover value is incremented to monitor how many times this is called.
  3. The stack count, and the handover counter are then checked they are the same.

##### ReplicatedStartupActor gym
* For QA workflows Test Replicated startup actor are correctly spawned on all clients
* Used to support QA test case "C1944 Replicated startup actors are correctly spawned on all clients" 
* Validation
  1. After two seconds checks that actor is visible to client and reports pass or fail

##### DestroyStartupActorGym gym
* For QA workflows Test Demonstrates that when a Level Actor is destroyed by server, a late connecting client does not see this Actor
* Used to support QA test case "C1945 - Stably named actors can be destroyed at runtime and late-connecting clients don't see them"
* Validation:
  1. At 10 seconds Actor is deleted and success message is shown notifying the PIE clients
  2. In late connecting client Actor cannot be seen, when user presses `F` keyboard button, then searches for Actor and returns success or failure message 

-----
2019-11-15: Page added with editorial review
