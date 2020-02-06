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
* To setup the gym, adjust the settings in "SpatialOS Settings -> Editor Settings -> Launch -> Launch configuration file options -> Server Workers" to include 4 servers in a 2x2 grid.
  * Set both "Rectangle grid column count" and "Rectangle grid row count" to 2
  * Set "Instances to launch in editor" to 4
* Adjust the setting "SpatialOS Settings -> Debug -> Spatial Debugger Class Path" to `BP_VerboseSpatialDebugger`.
* If it is working correctly the authority and authority intent of each cube can be seen to change as it moves across the floor.
* Press "L" to toggle locking actor migration.
* Press "K" to delete a cube in the scene (used for debugging actors deleted while locked).

##### Ability locking gym
* Demonstrates that an actor is locked from crossing servers while a gameplay ability is running on it.
* Contains a cube moving across a server boundary, with a gameplay ability granted to it which takes 4 seconds to complete.
* The ability counts from 1 to 5 over 4 seeconds, and sets a replicated variable on the cube. Whenever the value is changed, a text hovering above the actor is updated, to visualise the current value as replicated on the client.
* To setup the gym, adjust the settings in "SpatialOS Settings -> Editor Settings -> Launch -> Launch configuration file options -> Server Workers" to include 4 servers in a 2x2 grid.
  * Set both "Rectangle grid column count" and "Rectangle grid row count" to 2
  * Set "Instances to launch in editor" to 4
* Adjust the setting "SpatialOS Settings -> Debug -> Spatial Debugger Class Path" to `BP_VerboseSpatialDebugger`.
* If it is working correctly the authority and authority intent of the cube can be seen to change as it moves across the floor, and the text "Uninitialized" hovering over the cube.
* Press "T" to start the ability. The text above the cube should count from 1 to 5, and while the ability is running, the cube should not migrate to another server, even when it is physically in the authority region of another server.

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
  
##### Net reference test gym
* Test that references to replicated actors are stable when actors go in and out of relevance
* Properties referencing replicated actors are tracked. They are nulled when actors go out of relevance, and they should be restored when the referenced actor comes back into relevance.
* Validation :
  1. Cubes in a grid pattern hold references to their neighbours on replicated properties.
  2. A pawn is walking around with a custom checkout radius in order to have cubes go in and out of relevance
  3. The cube's color matches the number of valid references they have (red:0, yellow:1, green:2)
  4. If a cube does not have the expected amount of references to its neighbours, a red arror message will appear above.

##### ReplicatedStartupActor gym
* For QA workflows Test Replicated startup actor are correctly spawned on all clients
* Used to support QA test case "C1944 Replicated startup actors are correctly spawned on all clients"
* Validation
  1. After two seconds checks that actor is visible to client and reports pass or fail

##### DestroyStartupActorGym gym
* For QA workflows Test Demonstrates that when a Level Actor is destroyed by server, a late connecting client does not see this Actor.
* Used to support QA test case "C1945 - Stably named actors can be destroyed at runtime and late-connecting clients don't see them".
* Validation:
  1. At 10 seconds all cubes are deleted and success message is shown on all clients.
  2. Clients connecting after this point cannot see any cubes and, when pressing the `F` keyboard button, see a success or failure messages on screen.

##### WorkerFlagsGym gym
* Tests a fix for UNR-1259: Fix of the WorkerFlags data structure not being per worker. When running through Unreal Editor using single process, different worker types can read other worker type's flags. As a result flags of different worker types with the same name get the wrong value.
* Demonstrates that ClientWorkers and UnrealWorker read the correct value for the "test" worker flag, when both types have a "test" flag with different value.
* Validation:
  1. Use workerflags_testgym_config launch config to run multiplayer through the editor, for every worker running, different values based on their worker type should get printed/logged.

##### Soft references Test Gym
* Test what happens when we serialize soft references to asset, and references to assets not yet loaded on the client.
* This test should be ran from different processes for client and server (not enabling the option "Use single process" in the editor)
* It also enables the CVar net.AllowAsyncLoading to make sure references to packages loaded in the background get eventually resolved
* Validation :
  1. On play, soft references to green materials will be set on replicated properties (permutation of soft/hard references on single/array properties)
  2. Replicated properties will be picked up by RepNotifies on the client, and will set the received material on a cube in the scene
  3. Eventually, all cubes should turn green.
 
##### Net Owner Relevancy Test Gym
* Test that replicated Actors with the bUseNetOwnerRelevancy flag inherit from their Net Owner's relevancy
* NB : It only works if the GDK setting "Enable Net Cull Distance Interest" is enabled.
* Validation :
  1. On play, there is a cube with a large relevancy distance moving in the distance. Another one with a smaller distance will not be visible on the client, given the original distance from the viewer. This smaller actor has the bUseNetOwnerRelevancy set.
  2. Periodically (every 5 sec), the smaller actor will set/unset the bigger one as its net owner.
  3. The smaller actor should periodically appear and disappear on the client.
  
-----
2019-11-15: Page added with editorial review
