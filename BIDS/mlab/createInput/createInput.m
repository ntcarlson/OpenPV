%Path to k-wave toolkit
addpath('./k-Wave Toolbox');

%Set up kgrid world
Nx = DIM(1);
Ny = DIM(2);
assert(dx>0);
assert(dy>0);
kgrid = makeGrid(Nx, dx, Ny, dy);

%time array
kgrid.t_array = [0:dt:TIME_LENGTH];

t_end = length(kgrid.t_array);

%Pressure differences
source_pressure = WAVE_STRENGTH*sin(2*pi*WAVE_FREQUENCY*kgrid.t_array);

source_pressure = filterTimeSeries(kgrid, medium, source_pressure);

%Pressure mask, or drop position
source.p_mask = zeros(Nx, Ny);
source.p_mask(DROP_POS(2)-DROP_RADIUS:DROP_POS(2)+DROP_RADIUS,:) = 1;

% Preallocate an empty source matrix
num_source_positions = sum(source.p_mask(:));
source.p = zeros(num_source_positions,length(kgrid.t_array));

% move the source along the source mask by interpolating the pressure
% series between the source elements
sensor_index = 1;
t_index = 1;
while t_index < length(kgrid.t_array) && sensor_index < num_source_positions - 1
    % check if the source has moved to the next pair of grid points
    if kgrid.t_array(t_index) > (sensor_index*dy/SOURCE_VEL)
        sensor_index = sensor_index + 1;
    end    
    
    % calculate the position of source in between the two current grid
    % points
    exact_pos = (SOURCE_VEL*kgrid.t_array(t_index));
    discrete_pos = sensor_index*dy;
    pos_ratio = (discrete_pos - exact_pos) ./ dy;
    
    % update the pressure at the two current grid points using linear
    % interpolation
    source.p(sensor_index, t_index) = pos_ratio*source_pressure(t_index);
    source.p(sensor_index + 1, t_index) = (1 - pos_ratio)*source_pressure(t_index);
    
    % update the time index
    t_index = t_index + 1;
end

orig_drop = source.p_mask;

%Sensor mask
sensor = [];

%input arguments for movie recording
input_args = {'RecordMovie', true, 'MovieType', 'image', 'MovieName', MOVIE_NAME, 'PlotFreq', 1,'PlotPML',true,'PlotStim',false}; %%To plot movie
sensor_data = kspaceFirstOrder2D(kgrid, medium, source, sensor, input_args{:});
all_wave = sensor_data.p_plots_all;
count = sensor_data.count;
