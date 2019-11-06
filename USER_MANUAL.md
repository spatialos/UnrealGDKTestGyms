# SpatialOS GDK for Unreal Test Gyms Project 

#### What is a gym
* A gym is a level containing a minimal number of actors, with simple behaviours, necessary to demonstrate some peice of functionality in the Spatial GDK for Unreal.
* We use gyms to help quickly visualize if some feature is working.

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

New gym levels should be stored in `Content\Maps`.
New actors to be used in gyms should be stored in `Contetnt\Actors`.

#### How to add a gym 
1. Create a new level, with a descriptive name, and store it in the `Content\Maps` directory;
  * consider copying the `EmptyGym` level in `Content\Maps` as a starting point.
2. Consider adding a text object containing the gym name to the level;
  * this aids indentification from screen shots etc.
3. Populate the new level with actors needed to demonstrate the desired functionality;
  * prefer to use existing actors if possible,
  * if new actors are needed then store them in `Content\Actors`.
4. Add a description of the gym to this document;
  * breifly describe what it tests and how to use it. 

#### Current gyms

##### Empty gym
* This is used as a template for creating new gyms.

##### Handover gym
* Used to test that an entity can cross from one load balancing region to another.
* Contains a cube that moves back and forth across a floor.

-----
2019-11-06: Page added with limited editorial review
