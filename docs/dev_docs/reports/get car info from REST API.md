Use REST API to get more clear info about the player car class, name, and description.


The purpose is to get more informative car description that we currently do with  internal livery names (e.g., "#24 Some Car 1.50").


Make sure to make as little REST API calls as possible since these can crash the game.

# Implementation notes

To get the clean brand/manufacturer name for just the player's car, you need to combine data from the **Shared Memory** and the **REST API**. 

Here is the catch: **The REST API does not have an endpoint that returns *only* the player's car.** The endpoints `/rest/race/car` (used by both rF2 and LMU) return a JSON array containing *every* car currently loaded in the session. 

Therefore, to get just the player's info, your app must do the following:
1. Read the player's "ugly" vehicle name from the Shared Memory.
2. Fetch the full array of cars from the REST API.
3. Loop through the JSON array to find the object that matches the player's ugly name.
4. Extract the `manufacturer` field from that specific object.

Here are the more details, including a major quirk with how rFactor 2 formats names.

### 1. The REST API Endpoints
Here are the following endpoints (using a standard HTTP GET request):
*   **rFactor 2:** `http://localhost:5397/rest/race/car`
*   **Le Mans Ultimate (Primary):** `http://localhost:6397/rest/race/car`
*   **Le Mans Ultimate (Alternative):** `http://localhost:6397/rest/sessions/getAllVehicles`

### 2. The JSON Structure & The "Version Number" Quirk
When you hit `/rest/race/car`, you get a JSON array of objects. However, rFactor 2 and Le Mans Ultimate format this data differently, and rFactor 2 requires a specific workaround. Note: the current version of lmuFFB only supports LMU, so we can ignore rF2-specific details for now.

#### Le Mans Ultimate (LMU)
LMU is straightforward. The JSON objects contain a `desc` field and a `manufacturer` field.
```json[
  {
    "desc": "Ferrari 488 GTE Evo",
    "manufacturer": "Ferrari",
    ...
  }
]
```
**Matching Logic:** The `desc` field in the JSON perfectly matches the `mVehicleName` string you get from the Shared Memory.

#### rFactor 2 (rF2)
rFactor 2 is trickier. The JSON objects contain a `name` field, a `vehFile` field, and a `manufacturer` field.
```json[
  {
    "name": "#24 Some Car 1.50",
    "vehFile": "D:\\RF2\\Installed\\Vehicles\\SOMECAR\\1.50\\CAR_24.VEH",
    "manufacturer": "Ferrari",
    ...
  }
]
```
**The Quirk:** The `name` field in the REST API includes the installed mod version number at the end (e.g., `" 1.50"`). However, the `mVehicleName` provided by the Shared Memory **does not** include this version number (it will just be `"#24 Some Car"`). 

If you try to match them directly, they will fail. TinyPedal solves this by looking at the `vehFile` path, extracting the version number folder name, and slicing that exact number of characters off the end of the `name` string.


### Summary
Because the REST API is a "one-off" call that returns a heavy JSON payload of all cars, you should only run this HTTP request **once** when the player enters the track or when the session changes. Save the resulting `manufacturer` string in a variable in your app, and use that variable rather than querying the REST API every frame.


## Car class info

The good news is that **you do not need to use the REST API to get the car class.** 

The game provides the exact car class directly and reliably through the **Shared Memory API**, and it natively distinguishes between the different LMP2 variations.

### 1. Getting the Car Class from Shared Memory
In the Shared Memory plugin, the scoring data for each vehicle contains a specific variable for the class: `mVehicleClass`.

### 2. Distinguishing between LMP2 (WEC) and LMP2 (ELMS)
Because rFactor 2 and Le Mans Ultimate treat these as separate classes for scoring purposes, the `mVehicleClass` string will output them differently. 

The game outputs:
*   **`"LMP2"`** for the restricted WEC specification.
*   **`"LMP2_ELMS"`** for the unrestricted ELMS specification.

### Is there any point in using the REST API for car class info? No.
If you are already fetching the `/rest/race/car` JSON array to get the manufacturer (as discussed previously) and want to grab the class from there, you can. 

The JSON objects returned by that endpoint typically include a `category` string. For an LMP2 car, the category string usually looks something like this:
`"Oreca, LMP2, LMP2_ELMS"` or `"Le Mans Ultimate, LMP2"`.

However, parsing the `category` string from the REST API is messy because it often contains a comma-separated list of every filter tag applied to the car. 
