%% TVC Drone Physical Parameters (Estimated for 1-lb Coaxial Drone)
clear; clc;

% --- Mass & Weight ---
mass_lbs = 1.0; 
mass = mass_lbs * 0.453592; % Convert 1 lb to kilograms (~0.454 kg)
g = 9.81;                   % Gravity (m/s^2)
weight = mass * g;          % Weight force in Newtons (~4.45 N)

% --- Thrust ---
% For a stable hover, thrust must equal weight. 
% We assume max thrust is double the weight for acceleration head-room.
hover_thrust = weight;       
max_thrust = hover_thrust * 2; 

% --- Geometry & Inertia ---
% Let's assume a slender 12-inch tall (0.305m) cylindrical body.
length_body = 0.305; 
radius_body = 0.05;  % 2-inch radius (0.05m)

% Distance from Center of Mass (CoM) to the TVC Gimbal Pivot Point
% Assuming CoM is in the middle, and gimbal is at the very bottom
r_gimbal = length_body / 2; 

% Moments of Inertia (Approximated as a solid cylinder)
Ixx = 0.5 * mass * (radius_body^2);                 % Roll Inertia
Iyy = (1/12) * mass * (3*radius_body^2 + length_body^2); % Pitch Inertia
Izz = Iyy;                                          % Yaw Inertia (same as pitch)

% --- Actuator (Servo) Limits ---
max_gimbal_angle = 15;        % Maximum servo deflection in degrees
servo_speed_limit = 600;      % Servo speed limit in degrees/second
