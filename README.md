# Blaster

#### A 3rd person view fast-paced multiplayer shooter game built with c++ and Unreal Engine 5
![Thumbnail](https://user-images.githubusercontent.com/31377230/220782391-63a9c5bb-3eca-45f2-b595-02e0d1db586c.png)

## Features

#### Game

- Players can create and join steam sessions from all over the world.

- A peer-to-peer, client-server archeticture where the **listen server** maintains authority over important gameplay logic to prevent cheating and to replicate all relevant information to other clients.

- Using replication and RPCs only when needed and make use of unreal functions that are set to replicate built-in like Destroy().

- Create a lobby map where players join before traveling to the game map using seamless travel.

#### Animations

- Retarget animations for the unreal learning kit character to implement movement functionality. including Crouching, Jumping, Aiming, Throwing greandes. etc..
     > **Problem**: Turning-in-place animations imported from mixamo didn't have a root bone, and since turning is based on rotating the root bone, the character turn animation doesn't seem natural.                                                                                                                                         
     > **Solution**: Can add a root bone to the aniamtion's skeletal mesh in an external software like blender.
     
#### Weapons

- Players can have a primary and a secondary weapon to attach to their backpack and can swap between them.

- Create Weapon class which will be used as base to **projectile weapons** ( Assault rifles, Rocket launchers, Grenade launchers )                                     
and **HitScan weapons** that use line trace ( Shotgun, Sniper, SMG, pistol ).

- Shotguns/SMGs implement **random scatter**, making them ineffective in long range but lethal in close range combat.
     > **Problem**: Server is authoritative over the random scatter pattern. using client side prediction would result in two different scatter patterns between client and the server.                                                                                                                                                        
     > **Solution**: Make the client produce the scatter locally , then send an RPC to the server which in turn will inform all other clients. 
- Dynamic Crosshairs that spread and shrink and turn red when aiming at other players.
  
- Use FABRIK to always place the hand correctly on the weapon.

- Weapons eject bullet shells with physics and sound.

- Eliminated characters are dissolved with a dissolve material and a beam of light particle effect.

- Characters are respawned after elimination.

- Display all relevant data for the player on the HUD and update them only when a variable changes and not each frame.

- Use GameMode to create custom match states (Warmup, Cooldown) and create a match time that turns red when there's 30 seconds left. Restart game after Cooldown finishes.

- Use PlayerState to calculate player score and defeats and Use GameState to keep track of the top scoring players and announce the winner(s) at the end of each match.

- Match time is synced among all palyers so all display the time on the server.

#### Buffs & Pickups

- Buff the player with pickups that spawn randomly in different places to boost the player's attributes for a configurable amount of time.                             
    - **Health**: Increase player's Health.
    - **Shield**: Increase player'sShield.
    - **Speed**: Increase player's walk/crouch speed.
    - **Jump**: Increase player's jump velocity.
    - **Ammo**: Restore ammo for player's weapon type.
> **Problem**: What happens when no player interacts with the pickup and it's time to spawn another one in its place?                                                   
> **Solution**: Start the timer to spawn another pickup only when the first one got destroyed by attaching the timer to the pickups's OnDestroy() delegate. 

#### Lag Compenstion

- When firing, client doesn't wait for the server tp play the lcoal fire effects. but waits till the server spawn the actual bullet.

- Apply client-side prediction on Ammo count, reloading animation, Aiming.

## TODOs

#### Game

- Return to main menu & leaving the game.

- Player BookKeeping.

- Elimination announcements.

- Create a Teams Game mode.

- Preventing friendly fire.

- Create a Capture the flag Game mode. 

- Selecting match type.

#### Weapons

- Add swap animation between primary and secondary weapon.

- Apply headshot damage.

#### Lag Compenstion

- Apply server-side rewinding
