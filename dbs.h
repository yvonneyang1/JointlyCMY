#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <stdint.h>

#include "tiff.h"
#include "pxm.h"

#define MOD(X, Y)       (((X)%(Y) < 0)?(((X)%(Y)) + (Y)):((X)%(Y)))
#define ABS(X, Y)       (((X-Y) >= 0)?(X-Y):(Y-X))
#define MAX(x, y)       (((x) < (y)) ? (y) : (x))
#define MIN(x, y)       (((x) < (y)) ? (x) : (y))

 // Represents an image, where each data point is a double (typically represented as 64-bit.)
 // This struct is used for Cpe and cpp, as well as the error image.
struct doubleImage
{
    int  height;
    int  width;

    // The border size around the image that will be added for convolution and halftoning operations.
    int  borderSize;
    double **data;
};

// A property bag that holds the configuration of a specific run of the DBS Mono.
typedef struct Config
{
	// The full path to the continuous tone image to be halftone.
	// Acceptable extensions (case-sensitive) are: pbm, pgm, ppm, tif.
	char *inputImagePath;
	char *inputImagePath2;
	char *inputImagePath3;

	// The full path to the output halftone image. 
	// Acceptable extensions (case-sensitive) are: pbm, pgm, ppm, tif.
	char *outputImagePath;
	char *outputImagePath1;


	// The full path to the initial halftone image. If not present, the program will use randomly initialized image.
	// Acceptable extensions (case-sensitive) are: pbm, pgm, ppm, tif.
	char *initialHalftonePath;
	char *initialHalftonePath2;

	// Size of the matrix

	int MatrixSize;
	int MaxLevel;

	char *outputMatrixCPath;
	char *outputMatrixMPath;
	char *outputMatrixYPath;

	// A flag to enable toggling functionality in the DBS.
	int enableToggle;

	// A flag to enable swapping functionality in the DBS.
	int enableSwap;

//    // A flag to enable the use of wrap around in convolution and swap region assessment instead of mirroring and chopping.
//    int applyWrapAround;

	// The value of the Gamma to apply uncorrection. If equals to 1, no uncorrection will be applied.
	double gamma;
	
	// A value that determines the HVS filter shape, and it equals (view distance x resolution). Optimal value = 3500.
	int	scaleFactor;

	// A value that determines the HVS filter size. The final filter size will be (height x width), where each 
	// dimension = (4 x hvsSpreadSize + 1). Typical value = 23, which covers 99 % of the filter.
	int hvsSpreadSize;
	
	// The swap neighborhood size. The larger this value, the better the output, but the slower the performance. 
	// This needs to be an odd value. Typical value = 41, for a fast result.
	int swapSize;
	
	// The height of the block in which a single change is accepted. Typical value = 5. 
	int blockHeight;
	
	// The width of the block in which a single change is accepted. Typical value = 4.
	int blockWidth;

    // The maximum number of iterations, or passes, to run the DBS for an image. This value is used for DBS convergence.
    int maxIterationCount;

    // The minimum number of pixel changes in a DBS pass below which the algorithm will stop. This value is used for DBS convergence.
    int minAcceptableChangeCount;

	// A flag to enable printing different information, as well as saving the results of halftoning after each iteration.
	int enableVerboseDebugging;
} Config;

struct pxm_img* getInitialHalftone(char *imagePath, struct doubleImage *inputImage, double maxGrayValue, unsigned int seed);

void performCompleteDBSForScreenDesign(struct Config *config, struct doubleImage *inputImage,struct pxm_img *halftoneCMY,struct doubleImage *cpeCMY,
		struct pxm_img *halftoneC,struct doubleImage *cpeC, struct pxm_img *halftoneM, struct doubleImage *cpeM,
		struct pxm_img *beforeCMY,struct pxm_img *beforeC, struct pxm_img *beforeM, struct doubleImage *cpp, int stepIndex);


int runSinglePassDBS(struct Config *config, struct doubleImage *inputImage, struct pxm_img *halftoneCMY, struct doubleImage *cpeCMY,
		struct pxm_img *halftoneC, struct doubleImage *cpeC,struct pxm_img *halftoneM, struct doubleImage *cpeM,
		struct pxm_img *beforeCMY, struct pxm_img *beforeC, struct pxm_img *beforeM,
		struct doubleImage *cpp, uint8_t **blockStatusMatrix, int stepIndex);


//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

double getSwapDeltaErrorInRegion_1(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,struct pxm_img *halftoneM,
		struct doubleImage *cpeM, struct doubleImage *cpp,int rowIndex, int columnIndex, int *swapTargetRowIndex, int *swapTargetColumnIndex);

double getBestSwapInBlock_1(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC, struct pxm_img *halftoneM, struct doubleImage *cpeM,
		struct doubleImage *cpp,int blockRowIndex, int blockColumnIndex, int *bestChangeRowIndex, int *bestChangeColumnIndex,
		int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex);

void applySwap_1(struct Config* config, struct pxm_img* halftoneC, struct doubleImage* cpeC,struct pxm_img* halftoneM, struct doubleImage* cpeM,
		struct doubleImage* cpp, uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,int targetSwapRowIndex, int targetSwapColumnIndex);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

void applySwap_2(struct Config* config, struct pxm_img* halftone, struct doubleImage* cpe, struct doubleImage* cpp,
    uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,
    int targetSwapRowIndex, int targetSwapColumnIndex);

double getSwapDeltaErrorInRegion_2(struct Config *config, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
    int rowIndex, int columnIndex, int *swapTargetRowIndex, int *swapTargetColumnIndex);

double getBestSwapInBlock_2(struct Config *config, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
    int blockRowIndex, int blockColumnIndex, int *bestChangeRowIndex, int *bestChangeColumnIndex,
    int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex, struct pxm_img *beforeCMY);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void applySwap_3(struct Config* config, struct pxm_img* halftoneC, struct doubleImage* cpeC, struct pxm_img* halftoneM, struct doubleImage* cpeM,
		struct doubleImage* cpp, uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,
		int targetSwapRowIndex, int targetSwapColumnIndex);

double getSwapDeltaErrorInRegion_3(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM, struct doubleImage *cpp,int rowIndex, int columnIndex,
		int *swapTargetRowIndex, int *swapTargetColumnIndex, struct pxm_img *beforeC, struct pxm_img *beforeM );

double getBestSwapInBlock_3(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM,struct doubleImage *cpp,int blockRowIndex, int blockColumnIndex,
		int *bestChangeRowIndex, int *bestChangeColumnIndex,int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex,struct pxm_img *beforeC, struct pxm_img *beforeM);
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
struct pxm_img* mergePattern(struct pxm_img *differ,struct pxm_img *halftone, double ratio);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
struct pxm_img* findDifference(struct pxm_img *halftone, struct pxm_img *before, struct pxm_img *differ);

void separateCM(struct pxm_img *halftoneCM, struct pxm_img *halftoneC, struct pxm_img *halftoneM,double Cratio, double Mratio);

struct pxm_img* removeDots(struct pxm_img *halftone, unsigned int changelines,int modNum, int MatrixSize, int MaxLevel, int movNum, int screenNum);

struct pxm_img* addDots(struct pxm_img *halftone, int changeline, int modNum, int movNum);

//struct pxm_img* addDots(struct pxm_img *halftoneC, struct pxm_img *halftone, int MatrixSize, int MaxLevel, int changeline);

double countNum(struct pxm_img *halftone);

struct doubleImage* generateCTImage(struct pxm_img *halftone);

struct doubleImage *AllocateMatrix(int MatrixSize);

void updateMatrix(struct pxm_img *halftone, struct doubleImage *matrix, double currentlevel, double levelIndex, struct pxm_img* before);
//void updateMatrix_2(struct pxm_img *halftone, struct doubleImage *matrix, double currentlevel, struct pxm_img* before);
//void neighboringLevels(double currentlevel, struct doubleImage *matrix, double *levelup, double *leveldown, double *adjlevel, unsigned int MaximumLevel);

struct pxm_img* samepattern(struct pxm_img *halftone);






//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------


double getSwapDeltaError(struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
	int sourceRowIndex, int sourceColumnIndex, int targetRowIndex, int targetColumnIndex, int cppRowIndex, int cppColumnIndex);

double getToggleDeltaError(struct pxm_img *halftoneC, struct doubleImage *cpeC, double cppPeak, int rowIndex, int columnIndex);

double getBestToggleInBlock(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC, struct doubleImage *cpp,
                            int blockRowIndex, int blockColumnIndex, int *bestChangeRowIndex, int *bestChangeColumnIndex);

void applyToggle(struct Config* config, struct pxm_img* halftone, struct doubleImage* cpe, struct doubleImage* cpp,
                 uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex);

void updateCpe(struct Config *config, struct doubleImage * cpe, struct doubleImage *cpp, uint8_t **blockStatus, double a0, int rowIndex, int columnIndex);

struct doubleImage *generateCpp(struct doubleImage *psf);

struct doubleImage* generateHvsFunction(Config *config);

struct doubleImage* calculateCpe(struct doubleImage *inputImage, struct pxm_img *halftone, struct doubleImage* Cpp);

double calculateRmsError(struct doubleImage *inputImage, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp);

struct doubleImage* convolve(struct doubleImage *image, struct doubleImage *kernel);

void normalizeImage(struct doubleImage *image);

struct doubleImage* calculateErrorImage(struct doubleImage *inputImage, struct pxm_img *halftone);

void saveBasedImageToText(struct doubleImage *image, char *filePath);

void saveImageToText(double **data, int height, int width, char *filePath);

void saveHalftoneToText(uint8_t **data, int height, int width, char *filePath);

struct doubleImage* readDoubleImage(char *imagePath, double maxGrayValue, double gamma);

struct doubleImage* readTiffImage(char* imagePath);

struct doubleImage* readPxmImage(char* imagePath);

void writeHalftoneImage(struct pxm_img *halftone, char *imagePath);

void removeGammaCorrection(struct doubleImage *image, double gamma);

void deallocateShiftedImage(struct doubleImage *image);

