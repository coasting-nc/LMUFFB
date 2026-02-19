After report 3 is complete (with Gemini 3.1 Pro):

* ask in Ai studio to analyse it and determine if the various parts can be implemented in stages (to be infremental and to avoid too many changes at once), or if they are too interrelated and must be implemented together.
* ask if we could initially normalize only some components of the FFB signal. Specifically, I am interested more in normalizing steering rach, front axle forces, and real axle forces (SoP, etc.). I am not so interested in normalizing road textures, vibrations, and similar effects, at least initially. 
Is it possible to make this separation in the implementation?
* tell which separations are possible for staged implementation: parts of the solutions, and components of the FFB signal that will be normalized.