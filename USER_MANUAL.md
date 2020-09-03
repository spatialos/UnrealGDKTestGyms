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
Actors used in gyms are in `Content\Actors`: add any new Actors to this directory.

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

##### Dynamic Components gym
* Demonstrates that:
  * Dynamic component are correctly added to and removed from a replicated Actor (with load-balancing enabled).
* Contains:
  * A character with a PlayerController with key bindings for adding and removing a component.
* To test the gym:
    * Move the character until the inspector shows that the EntityACL and other server simulated components are on different workers
    * Press "R" to add the TestDynamicComponent to the character.
    * Observe the TestDynamicComponent added to the character's components list in the inspector, the ComponentPresence list increments in size, and the relevant component ID is also added to the EntityACL (you may need to search through generated schema to find the relevant component ID).
    * Press "T" to remove the TestDynamicComponent from the character.
    * Observe the TestDynamicComponent removed from the character's components list in the inspector, and the ComponentPresence list decrements in size.

##### Handover gym
* Demonstrates that:
  * Entities correctly migrate between area of authority.
* Contains:
  * A set of cubes that moves back and forth across a floor.
* To test the gym:
  * Observe the authority and authority intent of each cube can be seen to change as it moves across the floor.
  * Press "L" to toggle locking actor migration.
  * Press "K" to delete a cube in the scene (used for debugging actors deleted while locked).


##### Ability locking gym
* Demonstrates that:
  * An Actor will be locked from crossing servers while a gameplay ability is running on it.
  * A player ownership hierarchy of Actors (controller, character & state) will not migrate while one Actor in the hierarchy is locked.
* Contains:
  * a cube moving across a server boundary, with a gameplay ability granted to it which takes 4 seconds to complete.
  * a player with a gameplay ability granted to it which takes 4 seconds to complete.
* To test the gym:
  * If it is working correctly the authority and authority intent of the cube can be seen to change as it moves across the floor, and the text "Uninitialized" hovering over the cube.
  * Press "Q" to start the ability on the player.
  * Press "T" to start the ability on the cube.
  * The printing in the top-right should count from 1 to 5 over 4 seconds (and for the cube this is also visualized in the client).
  * While the ability is running, the player Actor group or cube should not migrate to another server, even when the relevant Actor is physically in the authority region of another server.

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
* Validation:
  1. On play, a replicated array of references to actors is filled with the map's content.
  2. Depending on how the operations are scheduled, some clients/server workers will receive null references (red log message).
  3. Eventually, after one or more RepNotify, all workers should receive all the valid references (green log message).

##### Net reference test gym
* NOTE: This gym also has an equivalent automated test. In order to run the test, follow the steps:
  1. Open the Session Frontend: Window -> Developer Tools -> Session Frontend.
  2. On the Automation tab, search for SpatialTestNetReference1, tick the box corresponding to it and hit Start Tests.
  3. The Session Frontend will then prompt you with the result of the test.
* Test that references to replicated actors are stable when actors go in and out of relevance
* Properties referencing replicated actors are tracked. They are nulled when actors go out of relevance, and they should be restored when the referenced actor comes back into relevance.
* Validation:
  1. Cubes in a grid pattern hold references to their neighbours on replicated properties.
  2. A pawn is walking around with a custom checkout radius in order to have cubes go in and out of relevance
  3. The cube's color matches the number of valid references they have (red:0, yellow:1, green:2)
  4. If a cube does not have the expected amount of references to its neighbours, a red error message will appear above.

##### ReplicatedStartupActor gym
* NOTE: This gym also has an equivalent automated test. In order to run the test, follow the steps:
  1. Open the Session Frontend: Window -> Developer Tools -> Session Frontend.
  2. On the Automation tab, search for SpatialTestReplicatedStartupActor1, tick the box corresponding to it and hit Start Tests.
  3. The Session Frontend will then prompt you with the result of the test.
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

##### Server to server RPC gym
* Demonstrates that:
  * Actors owned by different servers correctly send server-to-server RPCs.
* Contains:
  * A set of cubes placed in four quadrants of the level which:
    * Randomly send RPCs from each worker to the other cubes in the level.
    * Change colour to indicate which worker the owning worker just received an RPC from (where colours match the inspector colours used by the spatial debugger).
    * Show a count of how many RPCs have been received which is shown above the cube.
* If it is working correctly, the normally white cubes will start flashing the colours of the other workers in the level, and the counters above the cubes will turn the corresponding worker colours and increment for each RPC received. If not, the cubes will timeout waiting for RPCs to be received and this will be indicated above the cubes.

##### World composition gym
* Tests level loading and unloading.
* The gym contains a world with a set of marked areas on the floor with, denoting a level, containing a single actor, that an be loaded. Each area has a label in front describing the actor in the level.
* On starting the gym, move towards the any marked area text to load the associated level on the client. When it has been loaded it a cube will appear with the properties described by the level label.
* Moving away from the marked area will cause the level to be unloaded on the client. When it unloads the actor should disappear.
* Validation:
  1. Each level can be repeatedly loaded and unloaded on the client with no issue.

##### ServerTravel gym
* Demonstrates ServerTravel.
* The server will change the map for clients periodically. This can be verified by observing the change in map name.
* To test this you will need to change the following settings:
	"Edit -> Editor Preferences -> Level Editor -> Play - > Multiplayer Options -> Use Single Process" = false
	"Edit -> Editor Preferences -> Level Editor -> Play - > Multiplayer Options -> Editor Multiplayer Mode" = "Play As Client"
* Also ensure that you are not using zoning or any offloading.

##### Simulated GameplayTask gym
* Tests replicating simulated GameplayTasks to clients.
* Validation
  * Play with 2 clients connected.
  * Each connected client should display a log with "UTask_DelaySimulated::InitSimulatedTask".
  * There should not be logs saying "Error: Simulated task is simulating on the owning client".

##### Client Net Ownership gym
* Demonstrates that:
  * In a zoned environment, setting client net-ownership of an Actor correctly updates the `ComponentPresence` and `EntityACL` components, and allows server RPCs to be sent correctly.
* Contains:
  * A character with a `PlayerController` with key bindings for:
    * (Q) Making the client net-owner for the cube,
    * (R) Sending a server RPC from the client on the cube,
    * (T) Removing the client as the net-owner for the cube.
* To test the gym:
    * Press `Q` to make the client net-owner for the cube.
    * Observe that:
      * the `SpatialDebugger` authority icon updates to the virtual worker ID relating to the character.
      * in the inspector, the `owner` property on the cube entity is updated to the `PlayerController` and, in the list of component authorities for the cube entity, the `UnrealClientEndpoint` component (ID `9978`) is set to the client worker ID.
    * Press `R` to send a server RPC from the client on the cube.
    * Observe that:
      * the overhead value above the cube is incremented (this is set by the server and replicated to the client).
    * Press `T` to remove the client as net-owner for the cube.
    * Observe that:
      * pressing `R` no longer increments the overhead value,
      * in the inspector, the `owner` and `UnrealClientEndpoint` component authority assignments are both unset.

##### Server to server Take Damage RPC gym
* Tests AActor::TakeDamage.
* Contains a set of cubes placed in four quadrants of the level. AActor::TakeDamage is called twice on random cubes, once with a FPointDamageEvent input and once with a FRadialDamageEvent input. If the cube is not authoritative on the server a cross server RPCs will be called from AActor::TakeDamage. Upon receiving the RPCs the cube will display the HitLocation member of FPointDamageEvent and Origin member of FRadialDamageEvent.
* Adjust the setting "SpatialOS Settings -> Debug -> Spatial Debugger Class Path" to `BP_VerboseSpatialDebugger`.
* If it is working correctly, you will see "10 10 10" and "20 20 20" appear over the top of each cube intermittently. This represents the HitLocation data being sent using a cross server RPC inside a PointDamageEvent object and the Origin of RadialPointDamage event. 

##### Multiple Ownership gym
* Demonstrates sending RPCs on multiple actors that have their owner set to a player controller.
* Pressing "enter" will print out information on the client and server regarding the owners of each cube. Logs on the client will inform the user of the ownership state of pawns. Logs on the server will denote the successful attempt to send RPCs on certain pawns.
* Initially the player controller will not posses any pawn. This will mean that hitting "enter" will result in no server logs and client logs suggesting that no pawn is owned by the player controller.
* Pressing "space" will switch the possession between the two cubes in the gym. This action will also ensure that the unpossessed cube will still be owned by the player controller. If the player controller does not have possession of a pawn, "space" will simply posses one of the pawns.
* Ensure multi-worker is turned off.

#### FASAsyncGym
* Checks an edge case of the GDK handling of FastSerialized Arrays.
* Native Unreal prevents async asset loading causing null pointers in FAS (FastArraySerialization) callbacks.
* This test creates a situation where pointers to an asset will be replicated before the asset has been loaded on the client.
* When async loading completes the FAS callbacks will be called with valid pointers.
* How to test : 
  * Play the level.
  * If a green text saying "Replication happened, no null references" appears on the cube, the test passes.
  * Otherwise, a red text will be displayed, or other error messages.
* NOTE : This test should be ran with "Single Process" disabled in play settings to be valid.
  * "Edit -> Editor Preferences -> Level Editor -> Play - > Multiplayer Options -> Use Single Process" = false
* NOTE : Since this is using asynchronous asset loading, the editor should be restarted in between executions of this test.

##### Teleporting gym.
* Tests actor migration when load balancing is enabled.
* NOTE : This gym is likely to have random failures, as we are still working on load balancing.
* Known issues : UNR-3617, UNR-3790, UNR-3837, UNR-3833, UNR-411
* The gym is separated in 4 load balanced zones, and spawns a character which can teleport around.
* How to test : 
  * The character can walk around the center of the map, migrating between zones
  * Pressing T teleports the character to another zone.
    * This is a sharp transition far away from boundaries, to test when border interest is absent.
  * Pressing R spawns a new character in a different zone and posesses it.
    * This is a complex scenario to test what happens when an actor hierarchy is split over several zones.

-----
2019-11-15: Page added with editorial review
