function [act_time, ...
	  activity, ...
	  hist_activity, ...
	  hist_activity_bins, ...
	  pvp_header] = ...
      pvp_readActivity(layer, i_trial, hist_activity_bins, pvp_order)

  global NUM_BIN_PARAMS 
  global NUM_WGT_PARAMS

  global output_path 
  global N NROWS NCOLS % for the current layer
  global NFEATURES  % for the current layer
  global NO NK % for the current layer
  global first_i_trial last_i_trial num_i_trials
  global pvp_index
  global num_hist_activity_bins 

  if nargin < 2
    pvp_order = 1;
  end%%if

  pvp_fileTypes;

				% PetaVision always names spike files aN.pvp, where
				% N == layer index (starting at 0)
  filename = ['a', num2str(layer-1),'.pvp'];
  filename = [output_path, filename];

  disp([ 'reading activity from ', filename ]);
  
				%default return arguments
  activtiy = [];
  ave_rate = 0;

  if ~exist(filename,'file')
    disp(['~exist(filename,''file'') in pvp file: ', filename]);
    return;
  end%%if

  [pvp_header, pvp_index] = pvp_readHeader(filename);
  if isempty(pvp_header)
    disp(['isempty(pvp_header) in pvp file: ', filename]);
    return;
  end%%if

  file_type = pvp_header(pvp_index.FILE_TYPE);
  if ( file_type ~= PVP_NONSPIKING_ACT_FILE_TYPE )
    disp(['file_type ~= PVP_NONSPIKING_ACT_FILE_TYPE in pvp file: ', filename]);
  end%%if

  num_pvp_params = pvp_header(pvp_index.NUM_PARAMS);
  if ( num_pvp_params ~= 20 )
    disp(['num_pvp_params ~= 20 in pvp file: ', filename]);
  end%%if

  NCOLS = pvp_header(pvp_index.NX);
  NROWS = pvp_header(pvp_index.NY);
  NFEATURES = pvp_header(pvp_index.NF);
  N = NROWS * NCOLS * NFEATURES;
  NO = floor( NFEATURES / NK );

  fid = fopen(filename, 'r', 'native');

  pvp_header = fread(fid, NUM_BIN_PARAMS-2, 'int32');
  pvp_time = fread(fid, 1, 'double');

  i_trial_offset = i_trial * ( N + 2 ) * 4;  % N activity vals + double time
  pvp_status = fseek(fid, 4*(NUM_BIN_PARAMS-2), 'bof');  % read integer header
  pvp_status = fseek(fid, 8, 'cof'); % read time (double)
  pvp_status = fseek(fid, i_trial_offset, 'cof');
  if ( pvp_status == -1 )
    disp(['fseek(fid, i_trial_offset, ''cof'') == -1 in pvp file: ', filename]);
    return;
  end%%if

  act_time = fread(fid,1,'float64');
  disp(['act_time = ', num2str(act_time)]);
  [activity, countF] = fread(fid, N, 'float32');
  if countF ~= N
    disp(['countF ~= N:', 'countF = ', num2str(countF), '; N = ', num2str(N)]);
  end%%if
  min_activity = min(activity(:));
  max_activity = max(activity(:));
  disp(['min activity = ', num2str(min_activity)]);
  disp(['max activity = ', num2str(max_activity)]);
  nz_activity = sum(activity ~= 0);
  disp(['nz_activity = ', num2str(nz_activity)]);

  normalized_activity = ...
      ( activity( activity ~= 0 ) ) / 1; %...
  if isempty(hist_activity_bins) && numel(normalized_activity) > 0
    [hist_activity, hist_activity_bins] = ...
	hist( normalized_activity(:), num_hist_activity_bins);
  end%%if
  if numel(normalized_activity) > 0
    hist_activity = ...
	hist( normalized_activity(:), hist_activity_bins);
  else
    hist_activity = ...
	zeros(1,num_hist_activity_bins);
  end

  debug_readActivity = 0;
  if debug_readActivity
    fh_hist = figure;
    set(fh_hist, 'Name', ['hist(', num2str(layer), ',', num2str(i_trial), ')']);
    hist(activity(:));
  end%%if
  
  fclose(fid);
  
  activity = reshape( activity, [NFEATURES, NCOLS, NROWS] );
  if ~pvp_order
    activity = shiftdim( activity, [3, 2, 1] );
  end%%if

 






