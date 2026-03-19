# Remove REST API use for car brand detection #411

**Open**
**coasting-nc** opened this issue on Mar 18, 2026

## Description

Remove REST API use for car brand detection

REST API is actually not reliable for retrieving car brand information, since it returns the information of other vehicles (eg. it seems to always return the info for the Aston Martin GT3 car) rather than the current one.
Remove from the logs also all use of info about car brand obtained from rest api . To put car info in the logs (car brand, class, etc.), only use info from the shared memory .

Since the car brand / manufacturer is not explicitly available from the shared memory info (we have confirmed this through testing) we have confirmed this through testing) we will rely on other info like team names (like we do for the Porche with "Proton") to infer the car brand / manufacturer.
However, in this patch to remove the REST API usage related to car brands, we will not expand yet the logic to infer car brand from team names, we will just remove the usage of REST API.
