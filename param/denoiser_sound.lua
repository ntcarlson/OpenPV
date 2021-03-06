package.path = package.path .. ";" .. "/projects/pcsri/PetaVision/OpenPV/parameterWrapper/?.lua";
local pv = require "PVModule"


if #arg ~= 4 then
    print ("Usage: lua " .. arg[0] .. " <VThresh> <num features> <dictionary file> <trained data dir>"); 
    return(1);
end

local VThresh          = tonumber(arg[1]); --The threshold, or lambda, of the network
local dictionarySize   = tonumber(arg[2]);   --Number of patches/elements in dictionary 
local dictionaryFile   = arg[3];    --nil for initial weights, otherwise, specifies the weights file to load.
local basePath         = arg[4]
local NoiseStdDev      = 0.5;

local nbatch           = 20;    --Number of images to process in parallel
local displayPeriod    = 1000;   --Number of timesteps to find sparse approximation
local numEpochs        = 1;     --Number of times to run through dataset
local numImages        = 50; --Total number of images in dataset
local stopTime         = math.ceil((numImages  * numEpochs) / nbatch) * displayPeriod;
local outputPath       = basePath ..  "/test_" .. VThresh .. "/";
local checkpointDir    = outputPath .. "/Checkpoints";

-- Base table variable to store
local pvParameters = {
column = {
groupType = "HyPerCol";
    startTime                           = 0;
    dt                                  = 1;
    stopTime                            = stopTime;
    progressInterval                    = displayPeriod;
    writeProgressToErr                  = true;
    outputPath                          = outputPath;
    verifyWrites                        = false;
    checkpointWrite                     = false;
    lastCheckpointDir                   = checkpointDir; -- Disable checkpointing except on the final step
    suppressNonplasticCheckpoints       = false;
    numCheckpointsKept                  = 1;
    initializeFromCheckpointDir         = "";
    printParamsFilename                 = "testPhase8192.params";
    randomSeed                          = 1234567890;
    nx                                  = 128;
    ny                                  = 2;
    nbatch                              = nbatch;
    errorOnNotANumber                   = false;
};

Image = {
groupType = "PvpLayer";
    nxScale                             = 1;
    nyScale                             = 1;
    nf                                  = 512;
    phase                               = 0;
    mirrorBCflag                        = false;
    valueBC                             = 0;
    initializeFromCheckpointFlag        = false;
    writeStep                           = displayPeriod;
    initialWriteTime                    = displayPeriod;
    sparseLayer                         = false;
    updateGpu                           = false;
    dataType                            = nil;
    displayPeriod                       = displayPeriod;
    inputPath                           = "../chinese_opera/testoutmixp.pvp";
    offsetAnchor                        = "cc";
    offsetX                             = 0;
    offsetY                             = 0;
    autoResizeFlag                      = false;
    inverseFlag                         = false;
    normalizeLuminanceFlag              = true;
    normalizeStdDev                     = true;
    useInputBCflag                      = false;
    padValue                            = 0;
    batchMethod                         = "random";
    randomSeed                          = 123456789;
    writeFrameToTimestamp               = true;
};

NoiseLayer = {
    -- This layer calculates the difference between the reconstructed
    -- image and the input.
    groupType = "NoiseLayer";

    -- Scale and features match input layer
    nxScale                             = 1;
    nyScale                             = 1;
    nf                                  = 512;
    phase                               = 1;
    mirrorBCflag                        = false;
    valueBC                             = 0;
    initializeFromCheckpointFlag        = false;
    InitVType                           = "ZeroV"; 
    triggerLayerName                    = "Image";

    writeStep                           = -1;
    initialWriteTime                    = initialWriteTime;
    sparseLayer                         = false;
    updateGpu                           = false;
    dataType                            = nil;
    stdDev                              = NoiseStdDev;
    originalLayerName                   = "Image";
};

A1 = {
groupType = "HyPerLCALayer";
    nxScale                             = 0.5;
    nyScale                             = 0.5;
    nf                                  = dictionarySize;
    phase                               = 3;
    mirrorBCflag                        = false;
    valueBC                             = 0;
    initializeFromCheckpointFlag        = false;
    InitVType                           = "ConstantV";
    valueV                              = VThresh;
    triggerLayerName                    = nil;
    writeStep                           = displayPeriod;
    initialWriteTime                    = displayPeriod;
    sparseLayer                         = true;
    updateGpu                           = true;
    dataType                            = nil;
    VThresh                             = VThresh;
    AMin                                = 0;
    AMax                                = infinity;
    AShift                              = VThresh;
    VWidth                              = 0;
    timeConstantTau                     = 50;
    selfInteract                        = true;
    adaptiveTimeScaleProbe              = "AdaptiveTimeScales";
};

A1ErrorImage = {
groupType = "HyPerLayer";
    nxScale                             = 1;
    nyScale                             = 1;
    nf                                  = 512;
    phase                               = 2;
    mirrorBCflag                        = false;
    valueBC                             = 0;
    initializeFromCheckpointFlag        = false;
    InitVType                           = "ZeroV";
    triggerLayerName                    = NULL;
    writeStep                           = -1;
    sparseLayer                         = false;
    updateGpu                           = false;
    dataType                            = nil;
};

DenoiseError = {
  groupType                           = "HyPerLayer";
  nxScale                             = 1;
  nyScale                             = 1;
  nf                                  = 512;
  phase                               = 6;
  mirrorBCflag                        = false;
  valueBC                             = 0;
  initializeFromCheckpointFlag        = false;
  InitVType                           = "ZeroV";
  triggerLayerName                    = NULL;
  writeStep                           = displayPeriod;
  initialWriteTime                    = displayPeriod;
  sparseLayer                         = false;
  updateGpu                           = false;
  dataType                            = nil;
};

A1ReconImage = {
groupType = "HyPerLayer";
    nxScale                             = 1;
    nyScale                             = 1;
    nf                                  = 512;
    phase                               = 5;
    mirrorBCflag                        = false;
    valueBC                             = 0;
    initializeFromCheckpointFlag        = false;
    InitVType                           = "ZeroV";
    triggerLayerName                    = nil;
    writeStep                           = displayPeriod;
    initialWriteTime                    = displayPeriod;
    sparseLayer                         = false;
    updateGpu                           = false;
    dataType                            = nil;
};

A1ErrorImageToA1 = {
groupType = "TransposeConn";
    preLayerName                        = "A1ErrorImage";
    postLayerName                       = "A1";
    channelCode                         = 0;
    delay                               = {0.000000};
    convertRateToSpikeCount             = false;
    receiveGpu                          = true;
    updateGSynFromPostPerspective       = true;
    pvpatchAccumulateType               = "convolve";
    writeStep                           = -1;
    writeCompressedCheckpoints          = false;
    selfFlag                            = false;
    gpuGroupIdx                         = -1;
    weightSparsity                      = 0;
    originalConnName                    = "A1ToA1ErrorImage";
};

ImageToA1ErrorImage = {
groupType = "IdentConn";
    preLayerName                        = "NoiseLayer";
    postLayerName                       = "A1ErrorImage";
    channelCode                         = 0;
    delay                               = {0.000000};
    weightSparsity                      = 0;
};

ImageToDenoiseError = {
groupType = "IdentConn";
    preLayerName                        = "Image";
    postLayerName                       = "DenoiseError";
    channelCode                         = 0;
    delay                               = {0.000000};
    weightSparsity                      = 0;
};

A1ToA1ErrorImage = {
groupType = "MomentumConn";
    preLayerName                        = "A1";
    postLayerName                       = "A1ErrorImage";
    channelCode                         = -1;
    delay                               = {0.000000};
    numAxonalArbors                     = 1;
    plasticityFlag                      = false;
    convertRateToSpikeCount             = false;
    receiveGpu                          = false;
    sharedWeights                       = true;
    useListOfArborFiles                 = false;
    combineWeightFiles                  = false;
    wMinInit                            = -1;
    wMaxInit                            = 1;
    sparseFraction                      = 0.9;
    minNNZ                              = 0;
    initializeFromCheckpointFlag        = false;
    triggerLayerName                    = "Image";
    triggerOffset                       = 0;
    immediateWeightUpdate               = true;
    updateGSynFromPostPerspective       = false;
    pvpatchAccumulateType               = "convolve";
    writeStep                           = -1;
    writeCompressedCheckpoints          = false;
    selfFlag                            = false;
    combine_dW_with_W_flag              = false;
    nxp                                 = 16;
    nyp                                 = 2;
    nfp                                 = 512;
    shrinkPatches                       = false;
    normalizeMethod                     = "normalizeL2";
    strength                            = 1;
    normalizeArborsIndividually         = false;
    normalizeOnInitialize               = true;
    normalizeOnWeightUpdate             = true;
    rMinX                               = 0;
    rMinY                               = 0;
    nonnegativeConstraintFlag           = false;
    normalize_cutoff                    = 0;
    normalizeFromPostPerspective        = false;
    minL2NormTolerated                  = 0;
    dWMax                               = 0.5;
    normalizeDw                         = true;
    useMask                             = false;
    dWMaxDecayInterval                  = 0;
    dWMaxDecayFactor                    = 0;
    weightSparsity                      = 0;
    momentumMethod                      = "viscosity";
    momentumTau                         = 200;
    momentumDecay                       = 0;

    -- Use prior trained weights
    weightInitType                      = "FileWeight";
    initWeightsFile                     = dictionaryFile;
};

A1ReconImageToA1ErrorImage = {
groupType = "IdentConn";
    preLayerName                        = "A1ReconImage";
    postLayerName                       = "A1ErrorImage";
    channelCode                         = 1;
    delay                               = {0.000000};
    weightSparsity                      = 0;
};

A1ReconImageToDenoiseError = {
groupType = "IdentConn";
    preLayerName                        = "A1ReconImage";
    postLayerName                       = "DenoiseError";
    channelCode                         = 1;
    delay                               = {0.000000};
    weightSparsity                      = 0;
};

A1ToA1ReconImage = {
groupType = "CloneConn";
    preLayerName                        = "A1";
    postLayerName                       = "A1ReconImage";
    channelCode                         = 0;
    delay                               = {0.000000};
    convertRateToSpikeCount             = false;
    receiveGpu                          = false;
    updateGSynFromPostPerspective       = false;
    pvpatchAccumulateType               = "convolve";
    selfFlag                            = false;
    weightSparsity                      = 0;
    originalConnName                    = "A1ToA1ErrorImage";
};

AdaptiveTimeScales = {
groupType = "LogTimeScaleProbe";
    targetName                          = "EnergyProbe";
    message                             = nil;
    textOutputFlag                      = true;
    probeOutputFile                     = "AdaptiveTimeScales.txt";
    triggerLayerName                    = "Image";
    triggerOffset                       = 0;
    energyProbe                         = nil;
    baseMax                             = 0.011;
    baseMin                             = 0.01;
    tauFactor                           = 0.05;
    growthFactor                        = 0.05;
    writeTimeScales                     = true;
    writeTimeScaleFieldnames            = true;
    logThresh                           = 1;
    logSlope                            = 0.01;
};

EnergyProbe = {
groupType = "ColumnEnergyProbe";
    message                             = nil;
    textOutputFlag                      = true;
    probeOutputFile                     = "EnergyProbe.txt";
    triggerLayerName                    = nil;
    energyProbe                         = nil;
    reductionInterval                   = 0;
};

A1_L1Probe = {
groupType = "L1NormProbe";
    targetLayer                         = "A1";
    message                             = nil;
    textOutputFlag                      = false;
    triggerLayerName                    = nil;
    energyProbe                         = "EnergyProbe";
    coefficient                         = 0.5;
    maskLayerName                       = nil;
};

A1ErrorImageL2Probe_A1 = {
groupType = "L2NormProbe";
    targetLayer                         = "A1ErrorImage";
    message                             = nil;
    textOutputFlag                      = false;
    triggerLayerName                    = nil;
    energyProbe                         = "EnergyProbe";
    coefficient                         = 0.5;
    maskLayerName                       = nil;
    exponent                            = 2;
};

} --End of pvParameters

-- Print out PetaVision approved parameter file to the console
paramsFileString = pv.createParamsFileString(pvParameters)
io.write(paramsFileString)
