# SpatialOS GDK for Unreal Test Gyms Project 

#### What is a gym
*A gym is a level containing a minimal number of actors, with simple behaviours, necessary to demonstrate some peice of functionality in the Spatial GDK for Unreal.
*We use gyms to helps quickly, visualy identifiy if some feature is working.

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
|__Geometry
|
|__Maps
|
|__Meshes
|
|__Spatial
```

New gym levels should be store in `Content\Maps`.
New actors to be used in gyms should be stored in `Contetnt\Actors`.

#### How to add a gym 
* Create a new level, with a descriptive name, and store it in the `Content/Maps` directory;
  * consider copying the `EmptyGym` level in `Content/Maps` as a starting point.
* Consider adding a text object containing the gym name to the level;
  * this aids indentification from screen shots etc.
* Populate the new level with actors needed to demonstrate the desired functionality;
  * prefer to use existing actors if possible,
  * if new actors are needed then store them in `Content/Actors`.
