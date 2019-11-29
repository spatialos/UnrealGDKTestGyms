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

##### ServerTravel gym
* Demonstrates ServerTravel.
* The server will change the map for clients periodically. This can be verified by observing the change in map name.
* To test this you will need to change the following settings:
	"Edit -> Editor Preferences -> Level Editor -> Play - > Multiplayer Options -> Use Single Process" = false
	"Edit -> Editor Preferences -> Level Editor -> Play - > Multiplayer Options -> Editor Multiplayer Mode" = "Play As Client"
* Also ensure that you are not using zoning or any offloading.

-----
2019-11-06: Page added with editorial review
