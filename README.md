# Taxicab Game
This is a university project made by Giacomo Perlo and Alessio Mana in Turin in January 2021 for our Operating Systems Exam. 
We named it "TaxiCab Game". </br>
This is written in C *(standard 89)* and developed for Unix-like systems.
The main target was to build and run a program where the number of concurrent processes was the maximum possible for that execution.

## What is "Taxicab Game"?

![Example Image](https://drive.google.com/uc?export=view&id=1ttWKm6Utmnda7krZtkHu3dKhkAXgp2Xr)<br/>
In this game we are simulating a city (represented by a 2d map) with a certain number of *Taxi* and *Sources* which communicates each other. <br/>
- ***Sources*** : each *Source* is represented by a distinct process forked by a "Master process" (*Master.c*, which is the one who initialize the execution). Its position is fixed in the map and, with a certain frequence, it emits some request for a "*taxi services*" (with: start its own position and destination a random cell in the map).
- ***Taxi***: each *Taxi* is represented by a distinct process forked by a "Master process". It "born" in a random cell of the map and waits for a "*taxi service call*" from any source.<br/>

The main pourpose of the program is to maximize the number of taxi and sources alive simultaneously and to complete as many "*taxi service calls*" as possible. 

## Getting Started
What you need to execute this game is:
 - ***GCC*** compiler <br/>
    ``` sudo apt install gcc ```  <br> or <br> ``` sudo apt install build-essential``` 
 - ***Make*** util <br/>
    ``` sudo apt install make ```

## How do I run the game?
There is a ***Makefile*** which automatically compiles and runs the project:
- You just have to know two simple commands to use the Taxicab Game. Once you have opened a shell in the folder which contains the application files, write: <br/>
  - ``` make all``` to compile the application. (If you want to modify the Height and Width of the map you need to use this.) <br/>
  - ``` make run``` to run che application. (If you modify the Settings Parameters you just need to run the application with this command without recompile it.)<br/>
  - ``` CTRL-c``` if you want to force the termination of the game.

## Must know
The game use some parameteres:
- *Execution Parameters*: these are specified in the ***Settings*** file and will affect the result of the game. <br/>
  **If you modify them you don't have to recompile the application.** (Just write "make run" in the shell).
- *Compilation Parameters*: these are specified in the ***Common.h*** file (row: 26, 27) and affect the size of the map (height and width). <br/>
  **If you modify them you have to recompile the application.** (first you write "make all" and then, one compiled, "make run").


## For the most curious
This project was our first experience in C concurrent programming. It allowed us to learn the mechanisms behind this kind of programming and share some good time togheter. We take advantage of all those system calls which are essential in this kind of application. We also made experience on all the IPC such as Semaphores, Shared Memory and Message Queues. We enjoyed it a lot!
For sure something could be done in a better way (such as the algorithm which calculates the path a taxi have to follow to reach the destination/source) but we focused more over the "concurrent side" of the project.<br/>
For more examples, please refer to the [Documentation](https://drive.google.com/file/d/1nuqLbpuRCTUlv6wC8aB1Gj7PbQfNN3VL/view?usp=sharing).<br/>
Hope you enjoy this,<br/> 
Jack & Ale

## Contact
Giacomo Perlo: [Linkedin](https://www.linkedin.com/in/giacomo-perlo-1a81471b6/), <perlogiacomo@gmail.com><br/>
Alessio Mana: [Linkedin](https://www.linkedin.com/in/alessio-mana-051112175/), <alessioma20@gmail.com><br/> 
If you need help or want to know something more about all of this, we are ready and excited to help you!
