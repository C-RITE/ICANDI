#ifdef TRACKKERNELDLL_EXPORTS
#define TRACKKERNELDLL_API __declspec(dllexport) 
#else
#define TRACKKERNELDLL_API __declspec(dllimport) 
#endif

extern "C" __declspec(dllexport) int Call_CUDA_SetDevice(int deviceID);
extern "C" __declspec(dllexport) int Call_CUDA_GetDeviceCount(int *count);
extern "C" __declspec(dllexport) int Call_CUDA_GetDeviceNames(int deviceID, char *name);

extern "C" __declspec(dllexport) int Call_CUDA_FFT_init(STAB_PARAMS stab_params);
extern "C" __declspec(dllexport) int Call_CUDA_FFT_release();
extern "C" __declspec(dllexport) int Call_CUDA_ImagePatchK(int ofsX, int ofsY, bool bGlobalRef, int patchIdx);
extern "C" __declspec(dllexport) int Call_CUDA_GetPeakCoefs(float *coef_peak256, float *coef_peak128, float *coef_peak064);
extern "C" __declspec(dllexport) int Call_CUDA_SaccadeDetectionK(float coef_peak, int patchIdx, int *sx, int *sy);
extern "C" __declspec(dllexport) int Call_CUDA_GetCenter(float coef_peak1, float coef_peak2, bool jumpFlag, int ofsXOld, int ofsYOld, int *ofsX, int *ofsY);
extern "C" __declspec(dllexport) int Call_CUDA_GetPatchXY(float coef_peak, int BlockID, int centerX, int centerY, int *deltaX, int *deltaY);
extern "C" __declspec(dllexport) int Call_CUDA_FastConvF(unsigned char *imgO, unsigned char *imgI, int KernelID, int imgW, int imgH, bool bGlobalRef, bool bFilteredImg);
extern "C" __declspec(dllexport) int Call_CUDA_FastConvP(unsigned char *imgO, unsigned char *imgI, int KernelID, int imgW, int imgH, bool bGlobalRef, bool bEOF, int pid, int pcnt, int pheight, bool bRetImg);

//END HEADER FILE