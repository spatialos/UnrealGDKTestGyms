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
* Contains a set of cubes that moves back and forth across a floor.
* To setup the gym adjust the following settings in "SpatialOS Settings -> Runtime Settings -> Load Balancing":
  1. Check `Enable Unreal Load Balancer`.
  2. Set `Load Balancing Worker Type` to `UnrealWorker`.
  3. Set the `Load Balancing Strategy` to `BP_QuadrantLBStrategy`.
* Adjust the settings in "SpatialOS Settings -> Editor Settings -> Launch" to include 4 servers in a 2x2 grid.
* Adjust the setting "SpatialOS Settings -> Debug -> Spatial Debugger Class Path" to `BP_VerboseSpatialDebugger`.
* Override the PlayerController to be the LockingPlayerController. 
* If it is working correctly the authority and authority intent of each cube can be seen to change as it moves across the floor.

##### FASHandover gym
* Fast Array Serialization handover gym.
* Demonstrates that an actor with an ability system component can transition between workers correctly.
* Internally GAS uses Fast Array Serialization.
* Validation:
  1. A new GameplayEffect is added by the authoritative server on authority gained.
  2. Additionally a handover value is incremented to monitor how many times this is called.
  3. The stack count, and the handover counter are then checked they are the same.

##### Latency gym
* Gym for testing latency timing generations
* Requires access to Google's Stackdriver - see instructions in `SpatialLatencyTracer.h`
* Latency tests are run automatically per connected client
* To see results of the tests, go to https://console.cloud.google.com/traces/traces?project=holocentroid-aimful-6523579

##### Unresolved reference gym
* Test what happens when structs with references to actors whose entity have not been created yet are replicated. Replicating null references is accepted, but they should be resolved eventually.
* It is interesting when working with arrays, because unlike regular fields, we do not hold RepNotify until the reference is resolved (because we might never receive all of them)
* Validation :
  1. On play, a replicated array of references to actors is filled with the map's content.
  2. Depending on how the operations are scheduled, some clients/server workers will receive null references (red log message).
  3. Eventually, after one or more RepNotify, all workers should receive all the valid references (green log message).

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
  2. Clients connecting after delete also cannot see Actor and on `F` keyboard button press searches for Actor and returns success or failure message

##### WorkerFlagsGym gym
* Tests a fix for UNR-1259: Fix of the WorkerFlags data structure not being per worker. When running through Unreal Editor using single process, different worker types can read other worker type's flags. As a result flags of different worker types with the same name get the wrong value.
* Demonstrates that ClientWorkers and UnrealWorker read the correct value for the "test" worker flag, when both types have a "test" flag with different value.
* Validation:
  1. Use workerflags_testgym_config launch config to run multiplayer through the editor, for every worker running, different values based on their worker type should get printed/logged.
-----
2019-11-15: Page added with editorial review
