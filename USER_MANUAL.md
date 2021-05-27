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

### Current tests

#### Known issues

Some tests are currently failing and will have a "KNOWN_ISSUE" before their name.
The ReplicatedStartupActorTest is failing, pending [UNR-4305](https://improbableio.atlassian.net/browse/UNR-4305).

#### How to run the automated test gyms

Some test gyms can be run as automated tests. To discover and run these tests, follow [these steps](https://improbableio.atlassian.net/wiki/spaces/GBU/pages/1782644741/How+to+write+a+spatial+functional+test#How-to-run-the-test).

#### How to run the manual test gyms

##### Empty gym
* The template for creating new gyms. Copy this to use as a starting point for your own gym.

##### Dynamic Components gym

Deprecated, see [UNR-4809](https://improbableio.atlassian.net/browse/UNR-4809)

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
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* The manual version of the gym contains:
  * Four server workers arranged in a 2x2 grid.
  * Four cubes that moves back and forth across a floor, crossing server boundaries.
* Steps to run the manual version of the gym:
  * In the Unreal Editor's Content Browser, locate `Content/Maps/HandoverGym` and double click to open it.
  * In the Unreal Editor Toolbar, click Play to launch one client, one server worker and the SpatialOS runtime.
  * Note: Authority indicated by the number next to the `A` floating above the cubes. Authority intent is indicated by the number next to the `I`.
  * Observe that these values change when the cubes cross server boundaries.
  * Press `L` to lock actor migration. This setting is indicated by the padlock floating above the cubes.
  * Observe that authority and authority intent stop changing.
  * Press `K` to delete a cube in the scene.
  * Check that you can delete cubes with locking on and with locking off.
  * In the Unreal Editor Toolbar, click Stop when you're done.
  * Don't forget to check the Output Log to check that there are no errors.

##### Ability activation gym
* Demonstrates that:
  * Gameplay abilities can be activated across server boundaries.
  * An Actor will be locked from crossing servers while a gameplay ability is running on it.
  * A player ownership hierarchy of Actors (controller, character & state) will not migrate while one Actor in the hierarchy is locked.
* Contains:
  * a cube moving across a server boundary, with a gameplay ability granted to it which takes 4 seconds to complete.
  * a player with a gameplay ability granted to it which takes 4 seconds to complete.
* To test the gym:
  * If it is working correctly the authority and authority intent of the cube can be seen to change as it moves across the floor, and the text "Uninitialized" hovering over the cube.
  * Activate an ability on the player or the cube:
    * Press "Q" to activate the ability on the player.
    * Press "E","C" or "T" to activate the ability on the cube.
      * "E" activates the ability via gameplay event (CrossServerSendGameplayEventToActor).
      * "C" activates the ability by class (CrossServerTryActivateAbilityByClass).
      * "T" activates the ability by tag (CrossServerTryActivateAbilityByTag).
  * The ability should activate, and log messages in the top left should count from 1 to 5 over 4 seconds. If an ability was activated on the cube, the text above the cube should also change to show the count.
  * While the ability is running, the player Actor group or cube should not migrate to another server, even when the relevant Actor is physically in the authority region of another server.
  * Activating an ability on the cube should be possible whether the player and cube are authoritative on the same or different servers.
  * Press "V" to try and activate the ability on the cube without using the CrossServer methods.
    * The ability should activate when player and cube are on the same server.
    * The ability should not activate when player and cube are on different servers. A warning should be printed to the log that the ability could not be activated because the cube is a simulated proxy.


##### FASHandover gym
* Fast Array Serialization handover gym.
* Demonstrates that an actor with an ability system component can transition between workers correctly.
* Internally GAS uses Fast Array Serialization.
* Manual Steps:
  1. In the Unreal Editor's Content Browser, locate `Content/Maps/FASHandoverGym` and double click to open it.
  1. In the Unreal Editor Toolbar, click Play to launch one client.
  1. As soon as the game launches, a new GameplayEffect is added by the authoritative server on authority gained.
  1. `Effect Applied Correctly` and `Effect Persisted Correctly` should be printed in the client viewport and in the Output Log. If this happens, and there are no warns or errors, then the test has passed.

##### Latency gym
* NOTE: This gym runs nightly as an automated test in the [unrealgdk-nfr](https://buildkite.com/improbable/unrealgdk-nfr) pipeline. You should only run the gym manually if you're debugging that pipline. QA are not required to run this gym manually.
* This gym tests latency timing generation.
* To run it, you will requires access to Google's Stackdriver - see the instructions at [UnrealGDK/SpatialGDK/Source/SpatialGDK/Public/Utils/SpatialLatencyTracer.h#L52](https://github.com/spatialos/UnrealGDK/blob/master/SpatialGDK/Source/SpatialGDK/Public/Utils/SpatialLatencyTracer.h#L52).
* Latency tests are run automatically once per connected client.
* To see results of the tests, go to Google Stackdriver project [holocentroid-aimful-6523579](https://console.cloud.google.com/traces/traces?project=holocentroid-aimful-6523579).

##### Unresolved reference gym
* Tests what happens when structs with references to actors whose entity have not been created yet are replicated. Replicating null references is accepted, but they should be resolved eventually.
* It is interesting when working with arrays, because unlike regular fields, we do not hold RepNotify until the reference is resolved (because we might never receive all of them)
* Manual Steps:
  1. On play, a replicated array of references to actors is filled with the map's content.
  1. Depending on how the operations are scheduled, some clients/server workers will receive null references (red log message).
  1. Eventually, after one or more RepNotify, all workers should receive all the valid references (green log message).

##### Net reference test gym
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* This gym tests that references to replicated actors are stable when actors go in and out of relevance.
* Properties referencing replicated actors are tracked. They are nulled when actors go out of relevance, and they should be restored when the referenced actor comes back into relevance.
* Manual steps:
  1. Cubes in a grid pattern hold references to their neighbours on replicated properties.
  1. A pawn is walking around with a custom checkout radius in order to have cubes go in and out of relevance
  1. The cube's color matches the number of valid references they have (red:0, yellow:1, green:2)
  1. If a cube does not have the expected amount of references to its neighbours, a red error message will appear above.

##### ReplicatedStartupActor gym
* KNOWN ISSUE: The automated version of this test does not function: [UNR-4305](https://improbableio.atlassian.net/browse/UNR-4305)
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* This test gym verifies QA test case "C1944 Replicated startup actors are correctly spawned on all clients".
* Also verifies that startup actors correctly replicate arbitrary properties.
* Manual steps:
  * In the Unreal Editor's Content Browser, locate `Content/Maps/ReplicatedStartupActors` and double click to open it.
  * In the Unreal Editor Toolbar, click Play to launch one client, one server worker and the SpatialOS runtime.
  * After two seconds check the Unreal Editor's Output Log for `LogBlueprintUserMessages: [ReplicatedStartupActors_C_1] Client 1: Test passed`.
  * Also check for the absence of errors.
  * If the message is present and errors are absent, the test has passed.

##### DestroyStartupActorGym gym
* This gym demonstrates that when a Level Actor is destroyed by server, a late connecting client is unable to see this Actor.
* Manual steps:
  1. Open `Content/Maps/DestroyStartupActorsGym`.
  1. Select `Play` on the Unreal toolbar.
  1. After 10 seconds, all cubes in the map are deleted.
  1. Once this deletion has ocurred, locate `UnrealGDKTestGyms/LaunchClient.bat` and double click on it. This will launch a late connecting client.
  1. When the client has loaded, press the `F` keyboard button. A success message, `No actors found - Test Passed!`, should be printed in the client viewport.

##### WorkerFlagsGym gym
* Tests a fix for UNR-1259: Fix of the WorkerFlags data structure not being per worker. When running through Unreal Editor using single process, different worker types can read other worker type's flags. As a result flags of different worker types with the same name get the wrong value.
* Demonstrates that ClientWorkers and UnrealWorker read the correct value for the "test" worker flag, when both types have a "test" flag with different value.
* Validation:
  1. Open the WorkerFlagsGym map.
  1. Navigate to: Project Settings->SpatialOS GDK for Unreal->Editor Settings->Launch.
  1. Ensure "Auto-generate launch configuration file" is disabled.
  1. Set "Launch configuration file path" to `workerflags_testgym_config.json` (the file exists within the test gyms project).
  1. Navigate to: Editor Preferences->Level Editor->Play->Multiplayer Options.
  1. Ensure "Run Under One Process" is enabled.
  1. Play in editor with one client.
  1. Check that the server prints out the corresponding flag value from `workerflags_testgym_config.json` in the "Output Log" (15 by default).
  1. Check that the client prints out the corresponding flag value from `workerflags_testgym_config.json` in the "Output Log" (5 by default).

##### Server to server RPC gym
* This gym demonstrates that:
  * Actors owned by different servers correctly send server-to-server RPCs.
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* The manual gym contains:
  * A set of cubes placed in four quadrants of the level which:
    * Randomly send RPCs from each worker to the other cubes in the level.
    * Change colour to indicate which worker the owning worker just received an RPC from (where colours match the inspector colours used by the spatial debugger).
    * Show a count of how many RPCs have been received which is shown above the cube.
* Steps to run the gym manually: 
    * In the Content Browser, under Content, search for the Maps folder.
    * Open the ServerToServerRPCGym and hit Play.
    * If the gym is working correctly, the normally white cubes will start flashing the colours of the other workers in the level, and the counters above the cubes will turn the corresponding worker colours and    increment for each RPC received. If not, the cubes will timeout waiting for RPCs to be received and this will be indicated above the cubes.

##### World composition gym
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* Tests level loading and unloading.
* The gym contains a world with a set of marked areas on the floor with, denoting a level, containing a single actor, that an be loaded. Each area has a label in front describing the actor in the level.
* On starting the gym, move towards the any marked area text to load the associated level on the client. When it has been loaded it a cube will appear with the properties described by the level label.
* Moving away from the marked area will cause the level to be unloaded on the client. When it unloads the actor should disappear.
* Manual steps:
  1. Each level can be repeatedly loaded and unloaded on the client with no issue.

##### ServerTravel gym
* Known issue: Server travel is not supported, this gym will not pass until it's implemented by: [UNR-4270](https://improbableio.atlassian.net/browse/UNR-4270)
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
* This gym demonstrates that:
  * In a zoned environment, setting client net-ownership of an Actor correctly updates the `ComponentPresence` and `EntityACL` components, and allows server RPCs to be sent correctly.
* NOTE: This gym can be run both as an automated test and a manual one. To run it automatically, use [these steps](#how-to-run-the-automated-test-gyms).
* NOTE: If the automated test is successful, you will see a warning sign, instead of the usual green tick. This is the expected behaviour, and the log should start with: 'No owning connection for'...
* The manual gym contains:
  * A character with a `PlayerController` with key bindings for:
    * (Q) Making the client net-owner for the cube,
    * (R) Sending a server RPC from the client on the cube,
    * (T) Removing the client as the net-owner for the cube.
* Steps to run the gym manually:
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
* This gym demonstrates that:
  * `AActor::TakeDamage` functions.
* The manual gym contains:
  * A set of four cubes placed in the quadrants of the level.
* Steps to run the gym manually:
  * Adjust the setting `Project Settings -> SpatialOS GDK for Unreal - Runtime Settings -> Debug -> Spatial Debugger` to `BP_VerboseSpatialDebugger`.
  * Open `/Content/Maps/ServerToServerTakeDamageRPCGymCrossServer.umap`
  * Press Play.
  * If it is working correctly, you will see "10 10 10" and "20 20 20" appear over the top of each cube intermittently.
* If you'd like to know more: 
  * `AActor::TakeDamage` is called twice on random cubes, once with a `FPointDamageEvent` input and once with a `FRadialDamageEvent` input.
  * If the cube is not authoritative on the server a cross server RPCs will be called from `AActor::TakeDamage`. Upon receiving the RPCs the cube will display the `HitLocation` member of `FPointDamageEvent` and the `Origin` member of the `FRadialDamageEvent`.
  * "10 10 10" and "20 20 20" appearing over the top of each cube intermittently represents the `HitLocation` data being sent using a cross server RPC inside a `PointDamageEvent` object and the `Origin` of the `RadialPointDamage` event.

##### Multiple Ownership gym
* Map name: `Content/Maps/MultipleOwnershipGym.umap`
* Demonstrates sending RPCs on multiple actors that have their owner set to a player controller.
* To test the scenario follow these steps:
	1. Select `Play` on the Unreal toolbar.
	2. When your client had loaded, press `Enter` and check for logs printed in the client viewport. They should say the following:
	   "MultipleOwnershipCube has no owner"
	   "MultipleOwnershipCube2 has no owner"
	   At this point in the test the player controller doesn't posses a pawn. This is why hitting `Enter` results in no server logs, and in client logs suggesting that no pawn is owned by the player controller.
	3. Press `Space` once to possess one of the pawns.
	4. Press `Enter` and check the logs printed. They should say the following:
	   "MultipleOwnershipCube is owned by MultipleOwnershipController"
	   "RPC successfully called on MultipleOwnershipCube"
	   "MultipleOwnershipCube2 has no owner"
	   Pressing `Space` switched the possession between the two cubes in the gym.
	5. Press `Space` a second time to possess the second pawn.
	6. Press enter and check the logs printed. They should say the following:
	   "MultipleOwnershipCube2 is owned by MultipleOwnershipController"
	   "MultipleOwnershipCube is owned by MultipleOwnershipController"
	   "RPC successfully called on MultipleOwnershipCube2"
	   "RPC successfully called on MultipleOwnershipCube"

	Note: the order of the logs should not matter.

##### FASAsyncGym
* Checks an edge case of the GDK handling of FastSerialized Arrays.
* Native Unreal prevents async asset loading causing null pointers in FAS (FastArraySerialization) callbacks.
* This test creates a situation where pointers to an asset will be replicated before the asset has been loaded on the client.
* When async loading completes the FAS callbacks will be called with valid pointers.
* How to test : 
  * Go to `Edit > Editor Preferences > Level Editor - Play > Multiplayer Options > Run Under One Process`. Disable this option.
  * Play the level.
  * If a green text saying "Replication happened, no null references" appears on the cube, the test passes.
  * Otherwise, a red text will be displayed, or other error messages.
* NOTE : Since this is using asynchronous asset loading, the editor should be restarted in between executions of this test.

##### Teleporting gym
* Tests actor migration when load balancing is enabled.
* NOTE : This gym is likely to have random failures, as we are still working on load balancing.
* The gym is separated in 4 load balanced zones, and spawns a character which can teleport around.
* How to test :
  * In the Unreal Editor's Content Browser, locate `Content/Maps/TeleportGym` and double click to open it.
  * In the Unreal Editor Toolbar, click Play to launch the gym with one client connected.
  * Press T to teleport the character to another zone. Do this 5 times.
    * This is a sharp transition far away from boundaries, to test when border interest is absent.
  * Press R spawns a new character in a different zone and posesses it. Do this 5 times.
    * This is a complex scenario to test what happens when an actor hierarchy is split over several zones.
  * Pressing G spawns a GymCube. Spam this as much as you'd like.
    * Spawning the GymCube is currently used for testing hierarchy migration as it does not cause failure.
  * Locate a virtual worker boundary by running around. It's represented in the game world by a semi-transparent wall.
  * Run along that virtual worker boundary until you find the center of the map, where four virtual workers meet in a 2x2 configuration.
  * Run around in the center of the map, ensuring that you can cross the virtual worker boundaries seamlessly.
 
##### Spatial Debugger Config UI gym
* Tests that the `OnConfigUIClosed` callback can be set on the spatial debugger using blueprints.
* Gives an example of using that callback to return your game to the correct input mode after closing the debugger config UI, depending on whether your game itself had a UI open.
* Note: You may notice duing this test that, the Spatial Debugger config UI when you select Actor Tag Draw Mode: Local Player, the tag floating above the player's head disappears. This is expected behaviour, as the tag is visible in the top left of the client viewport.
* Manual Steps:
  * In the Unreal Editor's Content Browser, locate `Content/Maps/SpatialDebuggerConfigUIGym` and double click to open it.
  * In the Unreal Editor Toolbar, click Play to launch one client.
  * Press `U` on your keyboard to open the in-game UI.
  * Check that the mouse cursor appears and that you are able to click the button that has appeared.
  * Press `F9` to open the Spatial Debugger config UI.
  * Check that you are able to interact with that UI.
  * Press `F9` again to close the debugger config UI.
  * The mouse cursor should remain visible, and you should be able to click the button from earlier.
  * Press `U` to close the in-game UI.
  * The mouse cursor should now be captured by the game, meaning that it is not visible and when you move your mouse the camera in the game moves.
  * Press `F9` to open the debugger config UI again, this time without the game UI active. Check that you are able to interact with the config UI.
  * Press `F9` again to close the config UI. The game should capture the mouse again, and mouse movement should control the character camera like normal.

#### SpatialEventTracingTests
These test whether key trace events have the appropriate cause events. They can **only** be run automatically. To run them:
* Follow [these steps](#how-to-run-the-automated-test-gyms) to actually execute the tests.

##### Gameplay Cues gym
* Tests that gameplay cues get correctly activated on all clients.
* It includes a **non-instanced** gameplay cue that is triggered by pressing `Q` and visualised as **sparks** emitted from the controlled character.
* It also includes an **instanced** gameplay cue that is triggered by pressing `E` and visualised as a **cone** floating above the controlled character.
* Manual steps:
  * In the Unreal Editor's Content Browser, locate `Content/Maps/GameplayCuesMap` and double click to open it.
  * In the Unreal Editor Toolbar, click Play to launch two clients.
  * Position one client's character in view of the other client and in the same virtual worker boundary.
  * Press `Q` to trigger the non-instanced gameplay cue. A burst of sparks should be emitted from the controlled character, which should also be visible on the other client.
  * Both clients should print "Executed Gameplay Cue" to their client viewports. This will also be visible in the Output Log.
  * Press `E` to trigger the instanced gameplay cue. A cone should spawn above the controlled character and disappear after 2 seconds. The cone should be visible to both clients. Both clients should print "Added Gameplay Cue" to their client viewports. This will also be visible in the Output Log.
  * Now position the client in different virtual worker boundaries and re-test. The steps and outcomes should be identical. If they are, this test has passed.

##### Client Travel gym
* Tests that clients can travel from one cloud deployment to the same cloud deployment.
* How to test:
  * Navigate to `Edit > Project Settings > Project > Maps & Modes > Default Maps`.
  * If you can't see the `Server Default Map`, click `▽` to reveal more options.
  * Set `Server Default Map` to `ClientTravel_Gym`.
  * Select Cloud on the GDK toolbar.
  	* Enter your project name, and make up and enter an assembly name and a deployment name.
  	* Ensure that Automatically Generate Launch Configuration is checked.
  	* Ensure that Add simulated players is not checked.
  	* Ensure that the following options in the Assembly Configuration section are checked.
  	  * Build and Upload Assembly
  	  * Generate Schema
  	  * Generate Snapshot
  	  * Build Client Worker
  	  * Force Overwrite on Upload
  * From the GDK toolbar, select the dropdown next to the Start deployment button and ensure that `Connect to cloud deployment` is selected.
  * Click the Start deployment button.
  * When your deployment has started running, click Play in the Unreal Editor to connect a PIE client to your Cloud Deployment.
  * In the client, use the mouse and WASD to move the camera.
  * Press `K` to trigger a ClientTravel for the PlayerController to the same deployment. If you've moved your camera, pressing `K` should visibly snap the camera back to the position that the camera spawned in (indeed, it is spawning again). If this happens, the test has passed.

##### Multiworker World Composition gym
* Tests that servers without authoritive player controllers are still able to replicate relevant actors.
* Please note that when you run this test gym, you man notice the cubes moving discontinuously (that is, juddering or stuttering rather than moving smoothly). This is expected and should not be considered a defect. This occurs because, by default, with Replication Graph turned on, actors are only updated every third tick.
* Manual steps:
  * Before booting the Unreal Editor, open `UnrealGDKTestGyms\Game\Config\DefaultEngine.ini` and uncomment the `ReplicationDriverClassName` option by deleing the `;`.
  * Boot the Unreal Editor.
  * Open `Content/Maps/MultiworkerWorldComposition/MultiworkerWorldComposition.umap`.
  * Generate schema.
  * Play with 1 client.
  * In the client you should see two cubes moving back and forth over a worker boundary.
    * On each cross, the authority of the cube should switch to the appropriate server. If this happens, the test has passed.
  * Don't forget to open `UnrealGDKTestGyms\Game\Config\DefaultEngine.ini` and re-comment the `ReplicationDriverClassName` line.
  
##### Snapshot reloading test
Tests that snapshot reloading functions in local deloyments.<br>
**Note:** This test uses the `HandoverGym` as it saves.<br>
Manual steps:<br>
  1. `Edit > Project Settings > SpatialOS GDK for Unreal > Editor Settings > Launch > Auto-stop local deployment`. Select `Never`.
  1. `Edit > Project Settings > SpatialOS GDK for Unreal > Editor Settings > Play In Editor Settings > Delete dynamically spawned entities`. Uncheck this option.
  1. In the Unreal Editor's Content Browser, locate `Content/Maps/HandoverGym` and double click to open it.
  1. In the Unreal Editor Toolbar, click Play to launch one client.
  1. Stop the gym and note that the deployment is still running, as indicated by the state of the Stop Deployment button in the GDK Toolbar.
  1. Start the gym again and note that the level actors should be at their previous shutdown positions, not their original positions. You can do this repeatedly.
  1. The test has now passed.
  1. Don’t forget to revert the two settings changes you made before you run another test.

##### Ability Giving Gym
Tests that ability specs given to an `AbilitySystemComponent` on two different servers can be activated correctly via their handles.
* How to test:
  * Go to `Edit > Editor Preferences > Level Editor - Play > Multiplayer Options > Run Under One Process`. Disable this option.
  * Play with one client.
  * In the client, with your character still on the server that it spawned on, press `Q` on your keyboard.
    In the Command Prompt window that contains the server log output of the server that your player charachter is currently on, you should see the following two lines:
    > Giving and running ability with level 1
    > 
    > Ability activated on AbilityGivingGymCharacter_BP with Level 1
    
    Importantly, the level number stated in the two lines should match. 
  * Move the character to the other server and then press `E` on your keyboard.
    In the Command Prompt window that contains the server log output of the server that your player charachter is currently on, you should see the following two lines:
    > Giving and running ability with level 2
    >
    > Ability activated on AbilityGivingGymCharacter_BP with Level 2
    
    Again, the two level numbers should match. If they do, the test has passed.

##### Async Package Loading Gym
Tests that async package loading works when activated.

Manual steps:
  1. Open `UnrealGDKTestGyms\Game\Config\DefaultSpatialGDKSettings.ini`.
  1. Modify `bAsyncLoadNewClassesOnEntityCheckout` to `True`.
  1. Save and close `DefaultSpatialGDKSettings.ini`.
  1. Open the Unreal Editor.
  1. Open `Content/Maps/AsyncPackageLoadingGym.umap`.
  1. Play with one client and note that the in-world message saysL "Test waiting for success..."
  1. Launch an additional client by running the `UnrealGDKTestGyms\LaunchClient.bat` script.
  1. Note that the in-world message now says "Test passed!"
  1. The test has now passed. Don't forget to revert the settings change you made before you run another test.

What did I just validate?<br>
The late connecing client has validated the local state before sending the "Passed" message to the server. This check was done on `AAsyncPlayerController` and validated:
  1. Async loading config is enabled.
  1. That the client doesn't have loaded into memory the class we intend to async load.
  1. That the client eventually loads an actor instance of said class.


##### Soft references Test Gym
* Demonstrates that:
  * Soft references are correctly replicated.
  * Soft references that are serialized to an asset, and which reference assets that have not yet loaded on the client are correctly resolved using the CVar `net.AllowAsyncLoading`.
* Steps to run the manual version of the gym:
 1. Open `SoftReferenceTestGym.umap`
 1. Generate schema & snapshot.
 1. Navigate to: Editor Preferences->Level Editor->Play->Multiplayer Options.
 Ensure `Run Under One Process` is not checked.
 1. Select `Play` on the Unreal toolbar.
 1. Watch as the cubes turn green in under 8 seconds.

##### Player disconnect gym
* Demonstrates that:
  * Players are cleaned up correctly when they disconnect.
* Pre-test steps:
  * In the Unreal Editor's Content Browser, locate `Content/Maps/SpatialPlayerDisconnectMap` and double click to open it.
* How to test for a client disconnecting by returning to its main menu:
  * From the Unreal toolbar, open the Play drop-down menu and enter the number of players as 2.
  * Select Play.
  * From the UnrealGDK toolbar, open the Inspector.
  * Verify in Inspector that the following exist: Two client workers, two player controller entities (these are called `PlayerDisconnectController`) and two player character entities.
  * Press `M` in one of the clients, to make that client leave the deployment by traveling to the empty map.
  * Verify in the Inspector that only one client worker entity, one player controller entity and one player character entity exist.
  * In the Unreal Editor Toolbar, click Stop when you're done.
  * Shut down the deployment if this doesn't happen automatically.
* How to test for a player disconnecting by exiting the client window:
  * From the Unreal toolbar, open the Play drop-down menu and enter the number of players as 1.
  * Select Play.
  * Use `UnrealGDKTestGyms\LaunchClient.bat` script to launch a second client.
  * From the UnrealGDK toolbar, open the Inspector
  * Verify in Inspector that the following exist: Two client workers, two player controller entities (these are called `PlayerDisconnectController`) and two player character entities.
  * Close the window of the second client.
  * Verify in the Inspector that only one client worker entity, one player controller entity and one player character entity exist.
  * In the Unreal Editor Toolbar, click Stop when you're done.
  * Shut down the deployment if this doesn't happen automatically.
* These tests can also be run in the cloud by deploying the `PlayerDisconnectGym` map and launching two clients.

####RPCTimeoutTestGym
* Demonstrate that:
  * RPC parameters holding references to loadable assets are able to be resolved without timing out.
* NOTE: This gym can only be run manually (It requires tweaking settings and running in separate processes).
* Pre-test steps:
  * In the Unreal Editor's SpatialGDK runtime settings, set Replication->"Wait Time Before Processing Received RPC With Unresolved Refs" to 0
  * In Unreal's advanced play settings, set "Run Under One Process" to false
  * Set the number of players to 2
* Testing : 
  * Press play.
  * Two clients should connect, at least one outside of the editor process.
  * Controlled character should turn green after 2 sec.
  * If characters turn red, the test has failed.
  * If there is a red text reading : "ERROR : Material already loaded on client, test is invalid", check that this only happens on the client launched from within the editor
  * Clients connected from a separate process, or running the test from a fresh editor instance should not display this error message.

  ##### Visual Logger gym
* Tests that:
  * The Visual Logger displays multi worker logs accurately.
  * The Visual Logger correctly colour codes spatial and non-spatial logging objects.
  * The Visual Logger re-bases log times when loading multiple log files simultaneously.
  * TODO: The Visual Logger correctly loads and displays native Unreal log files.
* How to test:
  * Open `Content\Maps\VisualLogger\VisualLoggerManualTest.umap`
  * Open the Visual Logger window via menu `Window -> Developer Tools -> Visual Logger`
  * Click the `Play` button in the Unreal toolbar to start PIE session, with one client.
  * Click the `Start` button in the Visual Logger toolbar to start recording.
  * Run game for 60 seconds, and then stop game and Visual Logger.
  * Observe the following in the Visual Logger UI:
    * Two `StationaryGymCubes` will log continously, one log per tick.
    * Two `GymCubes` will log continously on the single client worker, one log per tick.
    * Two `GymCubes` will split their logs (one log per tick), across the two server workers, alternating as the cubes cross the zero-interest worker boundary.
    * The names of the `StationaryGymCubes` will be displayed in a different colour (default is `Blue`, see `Edit -> Editor Preferences -> Visual Logger -> Object Name Display Colors -> Other Objects`)
    * The names of the `GymCubes` will be displayed in a different colour (default is `Green`, see `Edit -> Editor Preferences -> Visual Logger -> Object Name Display Colors ->Replicated Actors`)
  * Save the log as `first.vlog`.
  * Clear the Visual Logger using the `Clear` button.
  * Repeat the process and save the log as `second.vlog`.
  * Clear the Visual Logger using the `Clear` button.
  * Set `Sync Log Timings Across Files` to false in `Edit -> Editor Preferences -> Visual Logger -> Sync Log Timings Across Files`.
  * In the Visual Logger click `Load` and load in `first.vlog`.
  * The file should successfully load and display expected logs.
  * In the Visual Logger click `Load` and load in `second.vlog`.
  * The file should successfully load and display expected logs. The contents of two log files should be displayed as if they were recorded at the same time.
  * Clear the Visual Logger using the `Clear` button.
  * In the Visual Logger click `Load` and load in `first.vlog` and `second.vlog` simultaneously, using the shift key to select both files.
  * The files should successfully load and display expected logs. The contents of two log files should be displayed as if they were recorded at the same time.
  * Clear the Visual Logger using the `Clear` button.
  * Set `Sync Log Timings Across Files` to true in `Edit -> Editor Preferences -> Visual Logger -> Sync Log Timings Across Files`.
  * In the Visual Logger click `Load` and load in `first.vlog`.
  * The file should successfully load and display expected logs.
  * In the Visual Logger click `Load` and load in `second.vlog`.
  * The file should successfully load and display expected logs. The contents of two log files should be displayed as if they were recorded at the same time.
  * Clear the Visual Logger using the `Clear` button.
  * In the Visual Logger click `Load` and load in `first.vlog` and `second.vlog` simultaneously, using the shift key to select both files.
  * The files should successfully load and display expected logs. The contents of two log files should be offset by the time delta between the two recordings.

-----
2019-11-15: Page added with editorial review
