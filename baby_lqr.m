clear; clc;

% Physical parameters ----------------------------------
mass      = 0.405;
g         = 9.81;
thrust_n  = mass * g; % enough thrust needed to hover
arm       = 0.205;
I_xx      = 0.01802468421;
I_yy      = I_xx; % symmetrical 

B_rx = (thrust_n * arm) / I_xx;
B_ry = (thrust_n * arm) / I_yy;

dt = 0.01; %100 hz

% Creating continous matrices ----------------------------

% [rx ry drx dry irx iry]
A_c = [0 0 1 0 0 0; % d(rx) / dt = drx
       0 0 0 1 0 0; % d(ry) / dt = dry
       0 0 0 0 0 0;
       0 0 0 0 0 0;
       1 0 0 0 0 0; % d(irx) / dt = rx
       0 1 0 0 0 0  % d(iry) / dt = ry
    ];

% [servo_X servo_Y]
B_c = [0     0;
       0     0;
       -B_rx 0;
       0     B_ry;
       0     0;
       0     0
    ];

% both are *continous* matrices that need to be converted to discrete

% ADC of the system ------------------------------------------------

%          I matrix for A_c, dimensions of B_c matrix
sys_c = ss(A_c, B_c, eye(6), zeros(6,2));
sys_d = c2d(sys_c, dt, 'zoh');

A_d = sys_d.A;
B_d = sys_d.B;

% both matrices are now discrete :D

% Controllability check --------------------------------------

% We really want the rank to equal the amount of states
% iirc its the unique solution, no free variables/states basically

Wc = ctrb(A_d, B_d);
rank_Wc = rank(Wc);
fprintf('Controllability rank: %d / %d', rank_Wc, size(A_d,1));
if rank_Wc == size(A_d,1)
    fprintf('  OK\n');
else
    fprintf('  WARNING: system is NOT fully controllable\n');
end

% Q and R functions ---------------------

% for now this is a diagonal matrix and will be elaborated more
% tired

Q6 = diag([ ...
    100, ... % rx
    100, ... % ry
    1, ... % drx
    100, ... % dry
    1, ... % irx
    1  ... % iry
  ]);

R = diag([ ...
    10, ... % servo_x (rad)
    10, ... % servo_y (rad)
  ]);

[K, P4, ~] = dlqr(A_d, B_d, Q6, R);

disp('K integral columns (signs now inherently mapped to plant physics):');
for ch = 1:2
    fprintf('  K(%d,%d) = %.8f\n', ch, 4+ch, K(ch, 4+ch));
end

disp(' ');
disp('const float K[LQR_MAX_INPUTS][LQR_MAX_STATES] = {');
for i = 1:2
    fprintf('    {');
    fprintf('%.8ff, ', K(i,1:5));
    fprintf('%.8ff', K(i,6));
    fprintf('},\n');
end
disp('};');

Acl_d = A_d - B_d * K;
eig_d = eig(Acl_d);
[~, idx] = sort(abs(eig_d), 'descend');
disp(' ');
disp('Closed-loop DISCRETE poles (magnitude must be < 1.0 for stability):');
for e = eig_d(idx).'
    fprintf('  %+.4f%+.4fi (Mag: %.4f)\n', real(e), imag(e), abs(e));
end


% these values are for the servo actuation for the simulation, we can
% further expand and change these for later if we differ from the original
% repo's numbers
u_min = [-0.149066, -0.149066];
u_max = [ 0.149066,  0.149066];
INTEGRAL_MAX = [1.0, 1.0];
x_ref6 = zeros(6,1);

A_c_phys = A_c(1:4, 1:4);
B_c_phys = B_c(1:4, :);

x0 = [deg2rad(15); deg2rad(10); 0; 0];   % [rx ry drx dry]
T  = 8.0;
[xs, igs, us, t, tu] = zoh_simulate(x0, zeros(2,1), T, dt, K, x_ref6, ...
    u_min, u_max, INTEGRAL_MAX, A_c_phys, B_c_phys);

figure('Position', [100 100 1200 600]);
sgtitle('LQI (6-state gimbal) | ZOH @ 100Hz');

% Angles
subplot(2,3,1);
plot(t, rad2deg(xs(:,1)), 'b', 'LineWidth', 2); hold on;
plot(t, rad2deg(xs(:,2)), 'r', 'LineWidth', 2);
yline(0, 'k--');
title('Angles (deg)'); legend('Angle X','Angle Y'); grid on; hold off;

% Rates
subplot(2,3,2);
plot(t, rad2deg(xs(:,3)), 'b', 'LineWidth', 2); hold on;
plot(t, rad2deg(xs(:,4)), 'r', 'LineWidth', 2);
yline(0, 'k--');
title('Rates (deg/s)'); legend('Rate X','Rate Y'); grid on; hold off;

% Integrals
subplot(2,3,3);
plot(t, igs(:,1), 'b', 'LineWidth', 2); hold on;
plot(t, igs(:,2), 'r', 'LineWidth', 2);
yline(0, 'k--');
title('Integrals (rad\cdot s)'); legend('irx','iry'); grid on; hold off;

% Servo outputs
subplot(2,3,4);
plot(tu, rad2deg(us(:,1)), 'b', 'LineWidth', 2); hold on;
plot(tu, rad2deg(us(:,2)), 'r', 'LineWidth', 2);
yline(rad2deg(u_max(1)), 'k:'); yline(rad2deg(u_min(1)), 'k:');
title('Servo Output (deg)'); legend('Servo X','Servo Y'); grid on; hold off;

% Phase portrait X
subplot(2,3,5);
plot(rad2deg(xs(:,1)), rad2deg(xs(:,3)), 'b', 'LineWidth', 1.5); hold on;
plot(rad2deg(xs(1,1)), rad2deg(xs(1,3)), 'go', 'MarkerFaceColor', 'g');
plot(0, 0, 'rx', 'MarkerSize', 10, 'LineWidth', 2);
title('Phase Portrait X'); xlabel('Angle (deg)'); ylabel('Rate (deg/s)'); grid on; hold off;

% Phase portrait Y
subplot(2,3,6);
plot(rad2deg(xs(:,2)), rad2deg(xs(:,4)), 'r', 'LineWidth', 1.5); hold on;
plot(rad2deg(xs(1,2)), rad2deg(xs(1,4)), 'go', 'MarkerFaceColor', 'g');
plot(0, 0, 'rx', 'MarkerSize', 10, 'LineWidth', 2);
title('Phase Portrait Y'); xlabel('Angle (deg)'); ylabel('Rate (deg/s)'); grid on; hold off;

% Shared x-axis label for the time plots
for sp = [1 2 3 4]
    subplot(2,3,sp); xlabel('Time (s)');
end

function [xs, igs, us, t, tu] = zoh_simulate(x0, integ0, T, dt, K, x_ref, u_min, u_max, INTEGRAL_MAX, A_p, B_p)
    n_steps = round(T / dt);
    nx = numel(x0); nu = numel(integ0);
    x = x0(:);  ig = integ0(:);
    xs = zeros(n_steps+1, nx);  igs = zeros(n_steps+1, nu);  us = zeros(n_steps, nu);
    xs(1,:) = x.';  igs(1,:) = ig.';

    for k = 1:n_steps
        err = [x; ig] - x_ref;
        u = -K * err;
        u_clamped = min(max(u, u_min.'), u_max.');

        for ch = 1:nu
            saturated = (u(ch) <= u_min(ch)*0.99) || (u(ch) >= u_max(ch)*0.99);
            if ~saturated
                ig(ch) = ig(ch) + err(ch) * dt;   % err(1:2) = rx, ry
            end
            ig(ch) = min(max(ig(ch), -INTEGRAL_MAX(ch)), INTEGRAL_MAX(ch));
        end

        us(k,:) = u_clamped.';
        [~, y] = ode45(@(tt, xx) A_p*xx + B_p*u_clamped, [0 dt], x);
        x = y(end,:).';
        xs(k+1,:) = x.';  igs(k+1,:) = ig.';
    end
    t  = linspace(0, T, n_steps+1).';
    tu = linspace(0, T-dt, n_steps).';
end