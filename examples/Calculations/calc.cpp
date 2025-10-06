/*
 [this_blog] == sailboatinstruments.blogspot.com
*/

#define PI 3.14159265
#define DEG_TO_RAD ((double)(PI/180.0))
#define RAD_TO_DEG ((double) (180.0/PI))

// Inputs

/* The measured apparent wind angle is the
   calibrated wind vane reading:
   [this_blog]/2011/10/new-wind-vane-calibration.html
*/
double awa_measured;  // -180 to 180 degrees

/* The offset is positive is the masthead unit misalignment is
   clockwise from above, and negative if the misalignment is
   counterclockwise from above. Obtained from calibration test run:
   [this_blog]/2011/02/corrections-to-apparent-wind-angle.html
*/
double offset;  // degrees, positive or negative

/* The measured boat speed is the speed sensor reading,
 [this_blog]/2011/03/measuring-boat-speed-part-1.html
 corrected by a calibration factor from a trial run:
 [this_blog]/2011/01/boat-and-wind-speed-calibration.html
*/
double meas_boat_speed;  // knots

/* The apparent wind speed is the speed sensor reading,
 corrected by a calibration factor from a trial run:
 [this_blog]/2011/01/boat-and-wind-speed-calibration.html
*/
double aws;

/* Heel is positive when the mast leans to starboard,
   and negative when the mast leans to port. Obtained
   from gyro compass.
*/
double heel;   // degrees, positive or negative

/* The leeway calibration factor K is obtained
   from a trial calibration run:
   [this_blog]/2011/02/leeway-calibration.html
*/
double K;

/* The magnetic heading is obtained from the
   gyrocompass reading, corrected for deviation:
   [this_blog]/2011/01/gyro-compass-calibration.html
*/
double heading;  // 0 to 360 deg (magnetic)

/* The COG (course over ground) is the true
   heading from the GPS
*/
double cog;  // 0 to 360 deg (true)

/* The SOG (speed over ground) is the boat speed
   measured by the GPS
*/
double sog;  // knots

/*  Magnetic variation: difference between true and magnetic North
*/
double variation;

// Outputs

double awa_offset;  // AWA corrected for offset (-180 to 180)
double awa_heel;  //   AWA corrected for heel (-180 to 180)
double leeway;    // leeway angle (degrees, positive or negative)
double stw;  // speed through water (knots)
double vmg;  // velocity made good (knots)
double tws;  // true wind speed (knots)
double twa;  // true wind angle (-180 to 180)
double wdir; // wind direction (0 to 360 deg, magnetic)
double soc;  // speed of current (knots)
double doc;  // direction of current (0 to 360 deg, magnetic)

// Correct awa for alignment offset
awa_offset = awa_measured + offset;
if(awa_offset > 180.0)
   awa_offset -= 360.0;
else if(awa_offset < -180.0)
   awa_offset += 360.0;

// Correct awa for heel
double tan_awa = tan(awa_offset * DEG_TO_RAD);
if(isnan(tan_awa))
   awa_heel = awa_offset;
else
{
   double cos_heel = cos(heel * DEG_TO_RAD);
   awa_heel = atan(tan_awa / cos_heel) * RAD_TO_DEG;
 
   if(awa_offset >= 0.0)
   {
      if(awa_offset > 90.0)
         awa_heel += 180.0;
    }
    else
    {
       if(awa_offset < -90.0)
          awa_heel -= 180.0;
    }
}

// Calculate leeway angle
if(meas_boat_speed == 0.0
 || (awa_heel > 0.0 && heel > 0.0)
 || (awa_heel < 0.0 && heel < 0.0))
        leeway = 0.0;
else
{
   leeway = K * heel / (meas_boat_speed * meas_boat_speed);
   // limit leeway value for very low speeds
   if(leeway > 45.0)
      leeway = 45.0;
   else if(leeway < -45.0)
      leeway = -45.0;
}

// Calculate STW (speed through water)
stw = meas_boat_speed / cos(leeway * DEG_TO_RAD);

// Calculate component of stw perpendicular to boat axis
double lateral_speed = stw * sin(leeway * DEG_TO_RAD);

// Calculate TWS (true wind speed)
double cartesian_awa = (270.0 - awa_heel) * DEG_TO_RAD;
double aws_x = aws * cos(cartesian_awa);
double aws_y = aws * sin(cartesian_awa);
double tws_x = aws_x + lateral_speed;
double tws_y = aws_y + meas_boat_speed;
tws = sqrt(tws_x * tws_x + tws_y * tws_y);

// Calculat TWA (true wind angle)
double twa_cartesian = atan2(tws_y, tws_x);
if(isnan(twa_cartesian)) // singularity
{
   if(tws_y < 0.0)
      twa = 180.0;
    else twa = 0.0;
}
else
{
   twa = 270.0 - twa_cartesian * RAD_TO_DEG;
   if(awa_heel >= 0.0)
      twa = fmod(twa, 360.0);
   else
      twa -= 360.0;

   if(twa > 180.0)
      twa -= 360.0;
   else if(twa < -180.0)
      twa += 360.0;
}

// Calculate VMG (velocity made good)
vmg = stw * cos((-twa + leeway) * DEG_TO_RAD);

// Calculate WDIR (wind direction)
wdir = heading + twa;
if(wdir > 360.0)
   wdir -= 360.0;
else if(wdir < 0.0)
   wdir += 360.0;

// Calculate SOC (speed of current)
double cog_mag = cog + variation;
double alpha = (90.0 - (heading + leeway)) * DEG_TO_RAD;
double gamma = (90.0 - cog_mag) * DEG_TO_RAD;
double curr_x = sog * cos(gamma) - stw * cos(alpha);
double curr_y = sog * sin(gamma) - stw * sin(alpha);
soc = sqrt(curr_x * curr_x + curr_y * curr_y);

// Calculate DOC (direction of current)
double doc_cartesian = atan2(curr_y, curr_x);
if(isnan(doc_cartesian))
{
   if(curr_y < 0.0)
      doc = 180.0;
    else doc = 0.0;
}
else
{
   doc = 90.0 - doc_cartesian * RAD_TO_DEG;
   if(doc > 360.0)
      doc -= 360.0;
   else if(doc < 0.0)
     doc += 360.0;
}