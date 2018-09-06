/******************************************************************
* file: dbs.c
* Authors: Yi Yang, Purdue University (built upon David Lieberman code)
* Date: Dec 2017
* Implementing: Jointly design Cyan, Magenta and Yellow screens using DBS
* It can write out matrix as "txt" or halftone pattern as "tif"
* Note that code can be easily changed
*******************************************************************/

#include "dbs.h"
#include <stdint.h>
#include "allocate.h"

// Performs a complete halftoning using DBS on the image whose initial halftone is passed.
void performCompleteDBSForScreenDesign(struct Config *config, struct doubleImage *inputImage,struct pxm_img *halftoneCMY,struct doubleImage *cpeCMY,
		struct pxm_img *halftoneC,struct doubleImage *cpeC, struct pxm_img *halftoneM, struct doubleImage *cpeM,
		struct pxm_img *beforeCMY,struct pxm_img *beforeC, struct pxm_img *beforeM, struct doubleImage *cpp, int stepIndex){

    // Generate block tracking elements.
    int rowBlockCount = (int) ceil((double) cpeC->height / (double) config->blockHeight);
    int columnBlockCount = (int) ceil((double) cpeC->width / (double) config->blockWidth);
    uint8_t **blockStatusMatrix = (uint8_t **) multialloc(sizeof(uint8_t), 2, rowBlockCount, columnBlockCount);

    // Enable all blocks to begin with.
    for (int i = 0; i < rowBlockCount; i++) {
        for (int j = 0; j < columnBlockCount; j++) {

            blockStatusMatrix[i][j] = (uint8_t) 1;
        }
    }

    // Run the passes until a convergnce condition is reached.
    for (int iterationIndex = 1; iterationIndex < config->maxIterationCount; iterationIndex++) {

        printf("%03d => ", iterationIndex);
        int totalChangeCount = runSinglePassDBS(config, inputImage, halftoneCMY, cpeCMY, halftoneC, cpeC, halftoneM, cpeM,
        		beforeCMY, beforeC, beforeM,cpp, blockStatusMatrix, stepIndex);



		if (config->enableVerboseDebugging) {

			// Write the intermediate halftone result.
			char fileName[100];
			sprintf(fileName, "halftone%02d.tif", iterationIndex);
			writeHalftoneImage(halftoneC, fileName);
		}

        if (totalChangeCount < config->minAcceptableChangeCount)
            break;
    }

    multifree((char *) blockStatusMatrix, 2);
}

// Runs a single pass DBS over the image to improve the given halftone, and returns the total number of changes.
int runSinglePassDBS(struct Config *config, struct doubleImage *inputImage, struct pxm_img *halftoneCMY, struct doubleImage *cpeCMY,
		struct pxm_img *halftoneC, struct doubleImage *cpeC,struct pxm_img *halftoneM, struct doubleImage *cpeM,
		struct pxm_img *beforeCMY, struct pxm_img *beforeC, struct pxm_img *beforeM,
		struct doubleImage *cpp, uint8_t **blockStatusMatrix, int stepIndex) {

    time_t blockStart;
    time_t blockEnd;

    // Capture the time at the beginning of the processing.
    time(&blockStart);

    int toggleCount = 0;
    int swapCount = 0;
    double deltaError = 0.0;

    // These values match the size of the status matrix.
    int rowBlockCount = (int) ceil((double) cpeC->height / (double) config->blockHeight);
    int columnBlockCount = (int) ceil((double) cpeC->width / (double) config->blockWidth);

    // Now, process all blocks.
    for (int i = 0; i < rowBlockCount; i++) {
        for (int j = 0; j < columnBlockCount; j++) {

            // No need to process the block if disabled.
            if (blockStatusMatrix[i][j] == 0) {
                continue;
            }

            int toggleRowIndex = -1;
            int toggleColumnIndex = -1;
            double toggleError = 0.0;

            if (config->enableToggle) {
                toggleError = getBestToggleInBlock(config, halftoneC, cpeC, cpp, i, j, &toggleRowIndex, &toggleColumnIndex);
            }

            int swapRowIndex = -1;
            int swapColumnIndex = -1;
            int swapTargetRowIndex = -1;
            int swapTargetColumnIndex = -1;
            double swapError = 0.0;

            if (config->enableSwap) {

            	// Judge which swap strategy should be applied

            	switch (stepIndex){

				case 1: swapError = getBestSwapInBlock_1(config, halftoneC, cpeC,halftoneM, cpeM, cpp,
                		i, j, &swapRowIndex, &swapColumnIndex,&swapTargetRowIndex, &swapTargetColumnIndex); break;

				case 2: swapError = getBestSwapInBlock_2(config, halftoneCMY, cpeCMY, cpp, i, j,
						&swapRowIndex, &swapColumnIndex,&swapTargetRowIndex, &swapTargetColumnIndex, beforeCMY); break;

				case 3: swapError = getBestSwapInBlock_3(config, halftoneC, cpeC,halftoneM, cpeM, cpp,i, j, &swapRowIndex, &swapColumnIndex,
						&swapTargetRowIndex, &swapTargetColumnIndex,beforeC,beforeM); break;

				default: printf(" uncorrect step 1 \n"); break;
				}


            }

			// No good result will result from either changes.
            if (toggleError >= 0.0 && swapError >= 0.0) {
                blockStatusMatrix[i][j] = 0;
                continue;
            }

            // First check the toggle. If both deltas are equal, prioritize toggle.
            if (toggleError <= swapError && toggleError < 0.0) {

                toggleCount++;
                blockStatusMatrix[i][j] = 1;
                deltaError += toggleError;

                applyToggle(config, halftoneC, cpeC, cpp, blockStatusMatrix, toggleRowIndex, toggleColumnIndex);
                continue;
            }

			// Second check the swap.
			if (swapError < toggleError && swapError < 0.0) {

                swapCount++;
                blockStatusMatrix[i][j] = 1;
                deltaError += swapError;

                switch (stepIndex){

				case 1: applySwap_1(config, halftoneC, cpeC, halftoneM, cpeM,cpp, blockStatusMatrix,
						swapRowIndex, swapColumnIndex,swapTargetRowIndex, swapTargetColumnIndex); break;

				case 2: applySwap_2(config, halftoneCMY, cpeCMY, cpp, blockStatusMatrix,
						swapRowIndex, swapColumnIndex,swapTargetRowIndex, swapTargetColumnIndex); break;

				case 3: applySwap_3(config, halftoneC, cpeC, halftoneM, cpeM,cpp, blockStatusMatrix,
						swapRowIndex, swapColumnIndex,swapTargetRowIndex, swapTargetColumnIndex); break;

				default: printf(" uncorrect step 2 \n"); break;
				}



            }
        }
    }

    int totalChangeCount = toggleCount + swapCount;
    double rmsError = calculateRmsError(inputImage, halftoneC, cpeC, cpp);
    
	// Capture the time at the end of the processing.
	time(&blockEnd);
	double duration = difftime(blockEnd, blockStart);
	
	printf("Toggles:%6d, Swaps:%6d, Total =%6d, DeltaError = %-.6f, RMS Error = %.6f, Duration = %4.2fsec\n",
			toggleCount, swapCount, totalChangeCount, deltaError, rmsError, duration);

    return totalChangeCount;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
// Applies a swap between the source and the target pixels, and updates the cpe matrix to reflect both changes.
void applySwap_1(struct Config* config, struct pxm_img* halftoneC, struct doubleImage* cpeC, struct pxm_img* halftoneM, struct doubleImage* cpeM,
		struct doubleImage* cpp, uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,
		int targetSwapRowIndex, int targetSwapColumnIndex) {

    // Guard againt bad location.
    if (bestChangeRowIndex < 0 || bestChangeRowIndex >= halftoneC->height ||
        bestChangeColumnIndex < 0 || bestChangeColumnIndex >= halftoneC->width ||
        targetSwapRowIndex < 0 || targetSwapRowIndex >= halftoneC->height ||
        targetSwapColumnIndex < 0 || targetSwapColumnIndex >= halftoneC->width) {

        fprintf(stderr, "Swap source or target indices are out of range.\n");
        return;
    }

    int pixel = halftoneC->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0 = pixel ? -1.0 : 1.0;

    int pixelM = halftoneM->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0M = pixelM ? -1.0 : 1.0;

    // The main pixel.
    halftoneC->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0 + pixel);
    updateCpe(config, cpeC, cpp, blockStatusMatrix, a0, bestChangeRowIndex, bestChangeColumnIndex);

    halftoneM->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0M + pixelM);
    updateCpe(config, cpeM, cpp, blockStatusMatrix, a0M, bestChangeRowIndex, bestChangeColumnIndex);

    // The target swap pixel.
    halftoneC->mono[targetSwapRowIndex][targetSwapColumnIndex] = pixel;
    updateCpe(config, cpeC, cpp, blockStatusMatrix, -a0, targetSwapRowIndex, targetSwapColumnIndex);

    halftoneM->mono[targetSwapRowIndex][targetSwapColumnIndex] = pixelM;
    updateCpe(config, cpeM, cpp, blockStatusMatrix, -a0M, targetSwapRowIndex, targetSwapColumnIndex);
}



// Evaluates and returns the delta error caused by swapping a specific pixel within a given neighborhood.
double getSwapDeltaErrorInRegion_1(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM, struct doubleImage *cpp,int rowIndex, int columnIndex,
		int *swapTargetRowIndex, int *swapTargetColumnIndex) {

    int pixel = halftoneC->mono[rowIndex][columnIndex];
    double minDeltaError = 0.0;

	// Integer division
	int size = config->swapSize / 2;
	int minRowIndex = rowIndex - size;
	int maxRowIndex = rowIndex + size;
	int minColumnIndex = columnIndex - size;
	int maxColumnIndex = columnIndex + size;

	// Find the best delta error in the swap region.
	// The loop indices are those of the block pixels.
    for (int i = minRowIndex; i <= maxRowIndex; i++) {
        for (int j = minColumnIndex; j <= maxColumnIndex; j++) {


			int targetRowIndex = MOD(i, cpeC->height);
			int targetColumnIndex = MOD(j, cpeC->width);

			if (halftoneM->mono[targetRowIndex][targetColumnIndex]==1){
				int cppRowIndex = abs(i - rowIndex);
				int cppColumnIndex = abs(j - columnIndex);
				double deltaErrorC = getSwapDeltaError(halftoneC, cpeC, cpp, rowIndex, columnIndex,
									targetRowIndex, targetColumnIndex, cppRowIndex, cppColumnIndex);
				double deltaErrorM = getSwapDeltaError(halftoneM, cpeM, cpp, rowIndex, columnIndex,
									targetRowIndex, targetColumnIndex, cppRowIndex, cppColumnIndex);

				double deltaError = deltaErrorC + deltaErrorM;

				if (deltaError < minDeltaError){
					*swapTargetRowIndex = targetRowIndex;
					*swapTargetColumnIndex = targetColumnIndex;

					minDeltaError = deltaError;
				}
			}
        }
    }

    return minDeltaError;
}

// Evaluates and returns the best swap delta error in a given block, and the pixel information that generates that error, 
// along with the related target swap pixel information.
double getBestSwapInBlock_1(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM,struct doubleImage *cpp,int blockRowIndex, int blockColumnIndex,
		int *bestChangeRowIndex, int *bestChangeColumnIndex,int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex) {

    int blockStartRowIndex = blockRowIndex * config->blockHeight;
    int blockStartColumnIndex = blockColumnIndex * config->blockWidth;

    int height = MIN(blockStartRowIndex + config->blockHeight, halftoneC->height);
    int width = MIN(blockStartColumnIndex + config->blockWidth, halftoneC->width);

    double minDeltaError = 0.0;

    int swapRowIndex = -1;
    int swapColumnIndex = -1;

    // Go over Block pixels.
    for (int i = blockStartRowIndex; i < height; i++) {
        for (int j = blockStartColumnIndex; j < width; j++) {
        	if (halftoneC->mono[i][j]==1){

        		double deltaError = getSwapDeltaErrorInRegion_1(config, halftoneC, cpeC,halftoneM, cpeM,
        				cpp, i, j, &swapRowIndex, &swapColumnIndex);
        		if(deltaError<minDeltaError){
        			*bestChangeRowIndex = i;
        			*bestChangeColumnIndex = j;
        			*bestSwapTargetRowIndex = swapRowIndex;
        			*bestSwapTargetColumnIndex = swapColumnIndex;
        			 minDeltaError = deltaError;
        		}
        	}
        }
    }
    return minDeltaError;
}





//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// Applies a swap between the source and the target pixels, and updates the cpe matrix to reflect both changes.
void applySwap_2(struct Config* config, struct pxm_img* halftone, struct doubleImage* cpe, struct doubleImage* cpp,
    uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,
    int targetSwapRowIndex, int targetSwapColumnIndex) {

    // Guard againt bad location.
    if (bestChangeRowIndex < 0 || bestChangeRowIndex >= halftone->height ||
        bestChangeColumnIndex < 0 || bestChangeColumnIndex >= halftone->width ||
        targetSwapRowIndex < 0 || targetSwapRowIndex >= halftone->height ||
        targetSwapColumnIndex < 0 || targetSwapColumnIndex >= halftone->width) {

        fprintf(stderr, "Swap source or target indices are out of range.\n");
        return;
    }

    int pixel = halftone->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0 = pixel ? -1.0 : 1.0;

    // The main pixel.
    halftone->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0 + pixel);

    updateCpe(config, cpe, cpp, blockStatusMatrix, a0, bestChangeRowIndex, bestChangeColumnIndex);

    // The target swap pixel.
    halftone->mono[targetSwapRowIndex][targetSwapColumnIndex] = pixel;
    updateCpe(config, cpe, cpp, blockStatusMatrix, -a0, targetSwapRowIndex, targetSwapColumnIndex);
}

// Evaluates and returns the delta error caused by swapping a specific pixel within a given neighborhood.
double getSwapDeltaErrorInRegion_2(struct Config *config, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
    int rowIndex, int columnIndex, int *swapTargetRowIndex, int *swapTargetColumnIndex) {

    int pixel = halftone->mono[rowIndex][columnIndex];
    double minDeltaError = 0.0;

	// Integer division
	int size = config->swapSize / 2;
	int minRowIndex = rowIndex - size;
	int maxRowIndex = rowIndex + size;
	int minColumnIndex = columnIndex - size;
	int maxColumnIndex = columnIndex + size;

	// Find the best delta error in the swap region.
	// The loop indices are those of the block pixels.
    for (int i = minRowIndex; i <= maxRowIndex; i++) {
        for (int j = minColumnIndex; j <= maxColumnIndex; j++) {

			int targetRowIndex = MOD(i, cpe->height);
			int targetColumnIndex = MOD(j, cpe->width);

        	// No need to work with pixels of the same value as the anchor.
            if (halftone->mono[targetRowIndex][targetColumnIndex] == pixel) continue;

            // Add a term if we are within the Cpp matrix.
        	// Note: This must be done on the original indices, not the wraparound ones.
            int cppRowIndex = abs(i - rowIndex);
            int cppColumnIndex = abs(j - columnIndex);

			double deltaError = getSwapDeltaError(halftone, cpe, cpp, rowIndex, columnIndex,
								targetRowIndex, targetColumnIndex, cppRowIndex, cppColumnIndex);

			if (deltaError < minDeltaError) {

                *swapTargetRowIndex = targetRowIndex;
                *swapTargetColumnIndex = targetColumnIndex;

                minDeltaError = deltaError;
            }
        }
    }

    return minDeltaError;
}

// Evaluates and returns the best swap delta error in a given block, and the pixel information that generates that error,
// along with the related target swap pixel information.
double getBestSwapInBlock_2(struct Config *config, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
    int blockRowIndex, int blockColumnIndex, int *bestChangeRowIndex, int *bestChangeColumnIndex,
    int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex,struct pxm_img *before) {

    int blockStartRowIndex = blockRowIndex * config->blockHeight;
    int blockStartColumnIndex = blockColumnIndex * config->blockWidth;

    int height = MIN(blockStartRowIndex + config->blockHeight, halftone->height);
    int width = MIN(blockStartColumnIndex + config->blockWidth, halftone->width);

    double minDeltaError = 0.0;

    int swapRowIndex = -1;
    int swapColumnIndex = -1;

    // Go over Block pixels.
    for (int i = blockStartRowIndex; i < height; i++) {
        for (int j = blockStartColumnIndex; j < width; j++) {

        	if(halftone->mono[i][j] != before->mono[i][j]){

        		double deltaError = getSwapDeltaErrorInRegion_2(config, halftone, cpe, cpp, i, j, &swapRowIndex, &swapColumnIndex);
        		if (deltaError < minDeltaError) {
        			*bestChangeRowIndex = i;
					*bestChangeColumnIndex = j;
					*bestSwapTargetRowIndex = swapRowIndex;
					*bestSwapTargetColumnIndex = swapColumnIndex;

					minDeltaError = deltaError;
        		}
        	}
        }
    }


    return minDeltaError;
}



//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
void applySwap_3(struct Config* config, struct pxm_img* halftoneC, struct doubleImage* cpeC, struct pxm_img* halftoneM, struct doubleImage* cpeM,
		struct doubleImage* cpp, uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex,
		int targetSwapRowIndex, int targetSwapColumnIndex) {

    // Guard againt bad location.
    if (bestChangeRowIndex < 0 || bestChangeRowIndex >= halftoneC->height ||
        bestChangeColumnIndex < 0 || bestChangeColumnIndex >= halftoneC->width ||
        targetSwapRowIndex < 0 || targetSwapRowIndex >= halftoneC->height ||
        targetSwapColumnIndex < 0 || targetSwapColumnIndex >= halftoneC->width) {

        fprintf(stderr, "Swap source or target indices are out of range.\n");
        return;
    }

    int pixel = halftoneC->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0 = pixel ? -1.0 : 1.0;

    int pixelM = halftoneM->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0M = pixelM ? -1.0 : 1.0;

    // The main pixel.
    halftoneC->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0 + pixel);
    updateCpe(config, cpeC, cpp, blockStatusMatrix, a0, bestChangeRowIndex, bestChangeColumnIndex);

    halftoneM->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0M + pixelM);
    updateCpe(config, cpeM, cpp, blockStatusMatrix, a0M, bestChangeRowIndex, bestChangeColumnIndex);

    // The target swap pixel.
    halftoneC->mono[targetSwapRowIndex][targetSwapColumnIndex] = pixel;
    updateCpe(config, cpeC, cpp, blockStatusMatrix, -a0, targetSwapRowIndex, targetSwapColumnIndex);

    halftoneM->mono[targetSwapRowIndex][targetSwapColumnIndex] = pixelM;
    updateCpe(config, cpeM, cpp, blockStatusMatrix, -a0M, targetSwapRowIndex, targetSwapColumnIndex);
}



// Evaluates and returns the delta error caused by swapping a specific pixel within a given neighborhood.
double getSwapDeltaErrorInRegion_3(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM, struct doubleImage *cpp,int rowIndex, int columnIndex,
		int *swapTargetRowIndex, int *swapTargetColumnIndex, struct pxm_img *beforeC, struct pxm_img *beforeM ) {

    int pixel = halftoneC->mono[rowIndex][columnIndex];
    double minDeltaError = 0.0;

	// Integer division
	int size = config->swapSize / 2;
	int minRowIndex = rowIndex - size;
	int maxRowIndex = rowIndex + size;
	int minColumnIndex = columnIndex - size;
	int maxColumnIndex = columnIndex + size;

	// Find the best delta error in the swap region.
	// The loop indices are those of the block pixels.
    for (int i = minRowIndex; i <= maxRowIndex; i++) {
        for (int j = minColumnIndex; j <= maxColumnIndex; j++) {


			int targetRowIndex = MOD(i, cpeC->height);
			int targetColumnIndex = MOD(j, cpeC->width);

			if (halftoneM->mono[targetRowIndex][targetColumnIndex]!=beforeM->mono[targetRowIndex][targetColumnIndex]){
				int cppRowIndex = abs(i - rowIndex);
				int cppColumnIndex = abs(j - columnIndex);
				double deltaErrorC = getSwapDeltaError(halftoneC, cpeC, cpp, rowIndex, columnIndex,
									targetRowIndex, targetColumnIndex, cppRowIndex, cppColumnIndex);
				double deltaErrorM = getSwapDeltaError(halftoneM, cpeM, cpp, rowIndex, columnIndex,
									targetRowIndex, targetColumnIndex, cppRowIndex, cppColumnIndex);

				double deltaError = deltaErrorC + deltaErrorM;

				if (deltaError < minDeltaError){
					*swapTargetRowIndex = targetRowIndex;
					*swapTargetColumnIndex = targetColumnIndex;

					minDeltaError = deltaError;
				}
			}
        }
    }

    return minDeltaError;
}

// Evaluates and returns the best swap delta error in a given block, and the pixel information that generates that error,
// along with the related target swap pixel information.
double getBestSwapInBlock_3(struct Config *config, struct pxm_img *halftoneC, struct doubleImage *cpeC,
		struct pxm_img *halftoneM, struct doubleImage *cpeM,struct doubleImage *cpp,int blockRowIndex, int blockColumnIndex,
		int *bestChangeRowIndex, int *bestChangeColumnIndex,int *bestSwapTargetRowIndex, int *bestSwapTargetColumnIndex,struct pxm_img *beforeC, struct pxm_img *beforeM ) {

    int blockStartRowIndex = blockRowIndex * config->blockHeight;
    int blockStartColumnIndex = blockColumnIndex * config->blockWidth;

    int height = MIN(blockStartRowIndex + config->blockHeight, halftoneC->height);
    int width = MIN(blockStartColumnIndex + config->blockWidth, halftoneC->width);

    double minDeltaError = 0.0;

    int swapRowIndex = -1;
    int swapColumnIndex = -1;

    // Go over Block pixels.
    for (int i = blockStartRowIndex; i < height; i++) {
        for (int j = blockStartColumnIndex; j < width; j++) {
        	if (halftoneC->mono[i][j]!= beforeC->mono[i][j]==1){

        		double deltaError = getSwapDeltaErrorInRegion_3(config, halftoneC, cpeC,halftoneM, cpeM,
        				cpp, i, j, &swapRowIndex, &swapColumnIndex, beforeC, beforeM);
        		if(deltaError<minDeltaError){
        			*bestChangeRowIndex = i;
        			*bestChangeColumnIndex = j;
        			*bestSwapTargetRowIndex = swapRowIndex;
        			*bestSwapTargetColumnIndex = swapColumnIndex;
        			 minDeltaError = deltaError;
        		}
        	}
        }
    }
    return minDeltaError;
}





//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// generate matrix

struct pxm_img* mergePattern(struct pxm_img *differ,struct pxm_img *halftone, double ratio)
{
	int count = 0;
	int count_2 = 0;

	for (int i = 0; i < differ->height; i++) {
		for (int j = 0; j < differ->width; j++){
			if (differ->mono[i][j] == 1){
				count ++;
			}
		}
	}
	double movDot = ceil(count*ratio);

	for (int i =0; i <halftone->height; i++){
		for (int j = 0; j<halftone->width; j++){
			if (differ->mono[i][j] == 1 && count_2<movDot){

				halftone->mono[i][j] = 1;
				differ->mono[i][j] = 0;

				count_2++;
			}
		}
	}
	return halftone;
}


struct pxm_img* combineDiffer(struct pxm_img *halftone, struct pxm_img *before, struct pxm_img *differ)
{
	int count = 0;
	//double randVal;

	for (int i =0; i <halftone->height; i++){
		for (int j = 0; j<halftone->width; j++){
			if (halftone->mono[i][j] != before->mono[i][j]){

				differ->mono[i][j] = 1;
				count++;
			}
			else {
				differ->mono[i][j] = 0;
				count++;
			}
		}
	}

	return differ;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

struct pxm_img* findDifference(struct pxm_img *halftone, struct pxm_img *before, struct pxm_img *differ)
{
	int count = 0;
	//double randVal;

	for (int i =0; i <halftone->height; i++){
		for (int j = 0; j<halftone->width; j++){
			if (halftone->mono[i][j] != before->mono[i][j]){

				differ->mono[i][j] = 1;
				count++;
			}
			else {
				differ->mono[i][j] = 0;
				count++;
			}
		}
	}

	return differ;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

void separateCM(struct pxm_img *halftoneCM, struct pxm_img *halftoneC, struct pxm_img *halftoneM, double Cratio, double Mratio){
	long count =0,count_1 = 0,count_2 = 0, randVal ;

	for (int i = 0; i < halftoneCM->height; i++) {
		for (int j = 0; j < halftoneCM->width; j++){
			if (halftoneCM->mono[i][j] == 1){
				count ++;
			}
		}
	}


	srand((unsigned) time(NULL));
	for (int i = 0; i < halftoneCM->height; i++) {
		for (int j = 0; j < halftoneCM->width; j++)
		{
			if (halftoneCM->mono[i][j] == 1)
			{
				randVal = rand() %100;
				if (count_1 < (count*Cratio) && count_2 < (count*Mratio)){
					if (randVal > 50){
						halftoneM->mono[i][j] = 1;
						halftoneC->mono[i][j] = 0;
						count_2 ++;
					}
					else
					{
						halftoneM->mono[i][j] = 0;
						halftoneC->mono[i][j] = 1;
						count_1 ++;

					}

				}
				else if (count_1 >= (count*Cratio)){

					halftoneM->mono[i][j] = 1;
					halftoneC->mono[i][j] = 0;
					count_2 ++;

				}
				else if(count_2 >= (count*Mratio)){
					halftoneM->mono[i][j] = 0;
					halftoneC->mono[i][j] = 1;
					count_1 ++;
				}
			}
			else
			{
				halftoneM->mono[i][j] = 0;
				halftoneC->mono[i][j] = 0;
			}
		}
	}
}

// remove dots
struct pxm_img* removeDots(struct pxm_img *halftone, unsigned int changelines, int modNum, int MatrixSize, int MaxLevel, int screenNum)
{
	int count = 0;
	//double randVal;

	double movNum = floor(MatrixSize*MatrixSize/MaxLevel);
	printf("movNum is %.5f\n", movNum);

	if (MOD(changelines , modNum) == 0){
		movNum = movNum + screenNum;
	}

	for (int i =0; i <halftone->height; i++){
		for (int j = 0; j<halftone->width; j++){
			if (halftone->mono[i][j] == 1 && count<movNum ){

				halftone->mono[i][j] = 0;
				count++;
			}
		}
	}


	return halftone;
}
// Add dots

struct pxm_img* addDots(struct pxm_img *halftone, int changeline, int modNum, int MatrixSize, int MaxLevel)
{
	double count = 0;

	double movNum = floor(MatrixSize*MatrixSize/MaxLevel);
	printf("movNum is %.5f\n", movNum);

	if (MOD(changeline , modNum) == 0){
			movNum = movNum +1;
		}
	//srand(randomizationSeed);


	for (int i =0; i <halftone->height; i++){
		for (int j = 0; j<halftone->width; j++){
			if(halftone->mono[i][j] ==0 && count<movNum){
					halftone->mono[i][j] = 1;
					count ++;
			}
		}
	}

	return halftone;
}



/*

struct pxm_img* addDots(struct pxm_img *halftoneC, struct pxm_img *halftone, int MatrixSize, int MaxLevel, int changeline)
{
	double count = 0, Ccount =0, Mcount = 0;
	double randVal;

	int movNum = ceil(MatrixSize*MatrixSize/MaxLevel);
	double threshold2 = (1/2)*MatrixSize*MatrixSize;

	printf("movNum = %d\n", movNum);

	if (MOD(changeline , 4) == 0){
			movNum = movNum +1;
		}
	//srand(randomizationSeed);

	double TotalNum = countNum(halftone);


	for (int i =1; i <halftone->height; i++){
		for (int j = 1; j<halftone->width; j++){
			if (TotalNum <= threshold2){
				if(halftone->mono[i][j] ==0 && halftoneC->mono[i][j] == 0 && count<movNum){
					halftone->mono[i][j] = 1;
					count ++;
				}
			}
			else
				if(halftone->mono[i][j] ==0 && count<movNum){
					halftone->mono[i][j] = 1;
					count ++;
				}
		}
	}

	return halftone;
}
*/



// check dots number

double countNum(struct pxm_img *halftone){

	double count = 0;
	for(int i = 0; i<halftone->height; i++){
		for(int j = 0; j<halftone->width; j++){
			if(halftone->mono[i][j] == 1){
				count++;
				}
			}
		}

	return count;

}

// generate C-T image based on the absorptance

struct doubleImage* generateCTImage(struct pxm_img *halftone) {


	int count = 0;

	struct doubleImage *inputImage = (struct doubleImage *)multialloc(sizeof(struct doubleImage),1, 1);
	inputImage->height = halftone->height;
	inputImage->width = halftone->width;
	inputImage->borderSize = 0;
	inputImage->data = (double **) multialloc(sizeof(double), 2, inputImage->height, inputImage->width);

	for(int i = 0; i<halftone->height; i++){
		for(int j = 0; j<halftone->width; j++){
			if(halftone->mono[i][j]==1){
				count = count++;

			}
		}
	}


	double absorptance = (double) count/(halftone->height*halftone->width);
	//printf("ab = %.5f \n", absorptance);
	//printf("count = %d \n", count);
	//printf("ab = %.3f \n", (double) count/(halftone->height*halftone->width));

	for (int i = 0; i < inputImage[0].height; i++) {
		for (int j = 0; j < inputImage[0].width; j++) {
			inputImage->data[i][j] = absorptance;
		}
	}

	return inputImage;
}


// Allocate matrix
struct doubleImage *AllocateMatrix(int MatrixSize){
	struct doubleImage *matrix = (struct doubleImage *)multialloc(sizeof(struct doubleImage),1,1);
	matrix->height = MatrixSize;
	matrix->width = MatrixSize;
	matrix->data = (double **)multialloc(sizeof(double),2, MatrixSize, MatrixSize);

	printf("Matrix (height, width) is (%d, %d)\n", matrix->height, matrix->width);

	for(int i = 0; i < matrix->height; i++){
		for (int j =0; j < matrix->width; j++){
			matrix->data[i][j] = 0;
		}
	}
    //test
	//printf("allotest:%d\n", matrix->data[1][1]);
	return matrix;
}

//update matrix

void updateMatrix(struct pxm_img *halftone, struct doubleImage *matrix, double currentlevel, double levelIndex, struct pxm_img* before){

	for(int i = 0 ; i < halftone->height; i++){
		for (int j = 0; j< halftone->width; j++){

			if (levelIndex == 1 ){
				if (halftone->mono[i][j] != before->mono[i][j] ){
					matrix->data[i][j] = (double)(currentlevel );
				}
			}
			else{
				if (halftone->mono[i][j] == 1){
					matrix->data[i][j] = (double)(currentlevel);
				}
			}
		}
	}
}


// Write out Matrix
void writeMatrix(struct doubleImage *matrix, char *outputMatrixPath){
	char outfile[100];
	sprintf(outfile, "%s", outputMatrixPath);
	FILE *fp =fopen(outfile, "wb");
	for (int i = 0; i < matrix->height; i++){
		for (int j = 0; j < matrix->width; j++){
			fprintf(fp, "%u\t", (unsigned int)matrix->data[i][j]);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
}


//difine a same halftone pattern
struct pxm_img* samepattern(struct pxm_img *halftone)
{
	struct pxm_img *samepattern = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);

	samepattern->height = halftone->height;
	samepattern->width = halftone->width;
	samepattern->pxm_type = 'g';
	samepattern->mono = (uint8_t **) get_img(samepattern->width, samepattern->height, sizeof(uint8_t));

	for(int i = 0; i < samepattern->height; i++){
		for (int j = 0; j < samepattern->width; j++){
			samepattern->mono[i][j] = halftone->mono[i][j];
		}
	}
	return samepattern;
}

// Find neighboring levels
/*
void neighboringLevels(double currentlevel, struct doubleImage *matrix, double *levelup, double *leveldown, double *adjlevel, unsigned int MaximumLevel){

	*levelup = (double)MaximumLevel;
	*leveldown = 0;
	for (int i=0; i< matrix->height; i++){
		for(int j = 0; j< matrix->width; j++){
			if (matrix->data[i][j] + 1 > currentlevel){
				if (((*levelup)>matrix->data[i][j])&&(currentlevel < matrix->data[i][j])){
					*levelup = matrix->data[i][j];
				}
			if (matrix[0]->data[i][j]<currentlevel){
				if (*leveldown < matrix[0]->data[i][j]){
					*leveldown = matrix[0]->data[i][j];
				}
			}
			}
		}
	}


	if (*levelup == (double)MaximumLevel){
		*adjlevel = *leveldown;}
	if (*leveldown == 0){
		*adjlevel = *levelup;}
	if ((currentlevel - *leveldown)<= (*levelup -currentlevel)){
		*adjlevel = *leveldown;
	}
	else
		*adjlevel = *levelup;


}


*/

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// Evaluates and returns the delta error caused by swapping a source pixel with a target pixel.
double getSwapDeltaError(struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
	int sourceRowIndex, int sourceColumnIndex, int targetRowIndex, int targetColumnIndex, int cppRowIndex, int cppColumnIndex) {

	int pixel = halftone->mono[sourceRowIndex][sourceColumnIndex];
	double a0 = pixel ? -1.0 : 1.0;
	double a1 = -2.0 * a0;

	double cppPeak = cpp->data[0][0];
	double cpeAtPixel = cpe->data[sourceRowIndex][sourceColumnIndex];

	double deltaError = 2.0 * cppPeak - 2.0 * a0 * cpeAtPixel - a1 * cpe->data[targetRowIndex][targetColumnIndex];

	if (cppRowIndex <= cpp->borderSize && cppColumnIndex <= cpp->borderSize) {
		deltaError += -2.0 * cpp->data[cppRowIndex][cppColumnIndex];
	}

	return deltaError;
}


// Applies a toggle to the given pixel, and updates the cpe matrix to reflect the change.
void applyToggle(struct Config* config, struct pxm_img* halftone, struct doubleImage* cpe, struct doubleImage* cpp,
    uint8_t** blockStatusMatrix, int bestChangeRowIndex, int bestChangeColumnIndex) {

    // Guard againt bad location.
    if (bestChangeRowIndex < 0 || bestChangeRowIndex >= halftone->height ||
        bestChangeColumnIndex < 0 || bestChangeColumnIndex >= halftone->width) {

        fprintf(stderr, "toggle index is out of range.\n");
        return;
    }

    int pixel = halftone->mono[bestChangeRowIndex][bestChangeColumnIndex];
    double a0 = pixel ? -1.0 : 1.0;

    halftone->mono[bestChangeRowIndex][bestChangeColumnIndex] = (uint8_t) (a0 + pixel);

    updateCpe(config, cpe, cpp, blockStatusMatrix, a0, bestChangeRowIndex, bestChangeColumnIndex);
}


// Evaluates and returns the delta error caused by toggling a specific pixel.
double getToggleDeltaError(struct pxm_img *halftone, struct doubleImage *cpe, double cppPeak, int rowIndex, int columnIndex) {

    int pixel = halftone->mono[rowIndex][columnIndex];
    double a0 = pixel ? -1.0 : 1.0;
    double toggleDeltaError = cppPeak - 2.0 * a0 * cpe->data[rowIndex][columnIndex];

    return toggleDeltaError;
}

// Evaluates and returns the best toggle delta error in a given block, and the pixel information that generates that error.
double getBestToggleInBlock(struct Config *config, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp,
    int blockRowIndex, int blockColumnIndex, int *bestChangeRowIndex, int *bestChangeColumnIndex) {

    int blockStartRowIndex = blockRowIndex * config->blockHeight;
    int blockStartColumnIndex = blockColumnIndex * config->blockWidth;

    int height = MIN(blockStartRowIndex + config->blockHeight, halftone->height);
    int width = MIN(blockStartColumnIndex + config->blockWidth, halftone->width);

    double cppPeak = cpp->data[0][0];

    double minDeltaError = 0.0;

    int changeRowIndex = -1;
    int changeColumnIndex = -1;

    // Go over Block pixels.
    for (int i = blockStartRowIndex; i < height; i++) {
        for (int j = blockStartColumnIndex; j < width; j++) {

			double deltaError = getToggleDeltaError(halftone, cpe, cppPeak, i, j);

            if (deltaError < minDeltaError) {

                changeRowIndex = i;
                changeColumnIndex = j;

                minDeltaError = deltaError;
            }
        }
    }

    *bestChangeRowIndex = changeRowIndex;
    *bestChangeColumnIndex = changeColumnIndex;

    return minDeltaError;
}




// Updates the Cpe matrix by adding/subtracting Cpp centered at the desired rowIndex, columnIndex, and then enables
// any block that may have been disabled.
void updateCpe(struct Config *config, struct doubleImage *cpe, struct doubleImage *cpp, uint8_t **blockStatusMatrix,
			   double a0, int rowIndex, int columnIndex) {

    for (int iCpp = -cpp->borderSize; iCpp <= cpp->borderSize; iCpp++) {
        for (int jCpp = -cpp->borderSize; jCpp <= cpp->borderSize; jCpp++) {

            int cpeRowIndex = MOD(rowIndex + iCpp, cpe->height);
            int cpeColumnIndex = MOD(columnIndex + jCpp, cpe->width);
			cpe->data[cpeRowIndex][cpeColumnIndex] -= a0 * cpp->data[iCpp][jCpp];
        }
    }

    // Enable blocks that have been touched by this change.
	int maxRowBlockCount = cpe->height / config->blockHeight;
    int maxColumnBlockCount = cpe->width / config->blockWidth;

    int minRow = MAX((rowIndex - cpp->borderSize) / config->blockHeight, 0);
    int minColumn = MAX((columnIndex - cpp->borderSize) / config->blockWidth, 0);

    int maxRow = MIN((rowIndex + cpp->borderSize) / config->blockHeight, maxRowBlockCount - 1);
    int maxColumn = MIN((columnIndex + cpp->borderSize) / config->blockWidth, maxColumnBlockCount - 1);

    // The loop indices are used inclusively.
    for (int i = minRow; i <= maxRow; i++) {
        for (int j = minColumn; j <= maxColumn; j++) {

            if (blockStatusMatrix[i][j] == (uint8_t) 0) {
                blockStatusMatrix[i][j] = (uint8_t) 1;
            }
        }
    }
}

// Reads the image from the given path (.pxm and .tif), if present, allocates and returns a pxm image, which is an integer image.
// If the image does not exit, we generate a random image.
struct pxm_img* getInitialHalftone(char *imagePath, struct doubleImage *inputImage, double maxGrayValue, unsigned int randomizationSeed) {
    
    struct pxm_img *halftone = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);

    halftone->height = inputImage->height;
    halftone->width = inputImage->width;
    halftone->pxm_type = 'g';
    halftone->mono = (uint8_t **) get_img(halftone->width, halftone->height, sizeof(uint8_t));

    if (strlen(imagePath) > 0) {
        struct doubleImage* image = readDoubleImage(imagePath, maxGrayValue, 1.0);

        // initialize all of image_double to input initial halftone
        for (int i = 0; i < halftone->height; i++) {
            for (int j = 0; j < halftone->width; j++) {
                halftone->mono[i][j] = (uint8_t) image->data[i][j];
            }
        }

        return halftone;
    }

    // Setup the randomization seed.
    srand(randomizationSeed);

    // If image path is not provided create and return a random halftone.	
    for (int i = 0; i < halftone->height; i++) {
        for (int j = 0; j < halftone->width; j++) {

            double randomNumber = (double) rand() / (double) RAND_MAX;
            double absorptance = inputImage->data[i][j];

            if (randomNumber <= absorptance)
                halftone->mono[i][j] = (uint8_t) 1;
            else
                halftone->mono[i][j] = (uint8_t) 0;
        }
    }

    return halftone;
}

// Save halftone image to disk.
void writeHalftoneImage(struct pxm_img *halftone, char *imagePath) {
    FILE * file;
    TIFF_img tiffImage;

    struct pxm_img *targetHalftone = (struct pxm_img*) multialloc(sizeof(struct pxm_img), 1, 1);;

    targetHalftone->height = halftone->height;
    targetHalftone->width = halftone->width;
    targetHalftone->pxm_type = 'g';
    targetHalftone->mono = (uint8_t **) get_img(halftone->width, halftone->height, sizeof(uint8_t));

    // Modify the image back to [0 - 255]
    for (int i = 0; i < halftone->height; i++) {
        for (int j = 0; j < halftone->width; j++) {
            targetHalftone->mono[i][j] = (uint8_t) (255 * (1 - halftone->mono[i][j]));
        }
    }

    // Check extension, and if pxm, invoke loading from the pxm file. Else, load from tiff.
    char extension[] = "    ";
    int length = strlen(imagePath);

    for (int i = 0; i < 4; i++) {
        extension[3 - i] = imagePath[length - i - 1];
    }

    // if imagePath is not .tif image, then it is a pxm image
    int isTiff = !strcmp(extension, ".tif");

    if (!isTiff) {

        file = fopen(imagePath, "w");
        if (file == NULL) {
            fprintf(stderr, "\nCan't write to file - %s\n", imagePath);
        }

        if (write_pxm(file, targetHalftone)) {
            fprintf(stderr, "\nCan't write pbm to file-%s\n", imagePath);
        }

        free_pxm(targetHalftone);

        fclose(file);
        return;
    }

    // If tiff is required, we will need to convert.
    get_TIFF(&tiffImage, targetHalftone->height, targetHalftone->width, 'g');

    /* initialize output image */
    for (int i = 0; i < targetHalftone->height; i++)
        for (int j = 0; j < targetHalftone->width; j++) {
            tiffImage.mono[i][j] = 0;
        }

    for (int i = 0; i < targetHalftone->height; i++) {
        for (int j = 0; j < targetHalftone->width; j++) {

            // Correcting to write image properly as 1 was considered dot earlier.
            tiffImage.mono[i][j] = (uint8_t) targetHalftone->mono[i][j];
        }
    }

    /* Write out halftone image */
    if (write_TIFF(imagePath, &tiffImage)) {
        fprintf(stderr, "error writing TIFF file %s\n", imagePath);
        exit(1);
    }

    free_pxm(targetHalftone);
    free_TIFF(&tiffImage);
}

// Calculate the filtered error between the input image and the halftone.
struct doubleImage* calculateCpe(struct doubleImage *inputImage, struct pxm_img *halftone, struct doubleImage* cpp) {

    struct doubleImage *errorImage = calculateErrorImage(inputImage, halftone);

    struct doubleImage *cpe = convolve(errorImage, cpp);

    // Clean up.
    multifree((double *) errorImage->data, 2);
    free(errorImage);

    return cpe;
}

// Calculates the RMS error between the input image and the halftone.
double calculateRmsError(struct doubleImage *inputImage, struct pxm_img *halftone, struct doubleImage *cpe, struct doubleImage *cpp) {

	double error = 0.0;

	// Get new error between the input image and the current halftone.
	struct doubleImage *errorImage = calculateErrorImage(inputImage, halftone);

	for (int i = 0; i < cpe->height; i++) {
		for (int j = 0; j < cpe->width; j++) {
			
			error += cpe->data[i][j] * errorImage->data[i][j];
		}
	}

	double rms = sqrt(error / (cpe->height * cpe->width));
	
	multifree((double *) errorImage->data, 2);
	free(errorImage);

	return rms;
}

// Convolves the image with the passed kernel.
struct doubleImage* convolve(struct doubleImage *image, struct doubleImage *kernel) {

    struct doubleImage * cpe = (struct doubleImage *) multialloc(sizeof(struct doubleImage), 1, 1);
    cpe->width = image->width;
    cpe->height = image->height;
    cpe->borderSize = 0;
    cpe->data = (double **) multialloc(sizeof(double), 2, cpe->height, cpe->width);

    // Convolve the error image with Cpp.
    for (int iCpe = 0; iCpe < cpe->height; iCpe++) {
        for (int jCpe = 0; jCpe < cpe->width; jCpe++) {

            // Initialize to 0.
            cpe->data[iCpe][jCpe] = 0.0;

            for (int iCpp = -kernel->borderSize; iCpp <= kernel->borderSize; iCpp++) {
                for (int jCpp = -kernel->borderSize; jCpp <= kernel->borderSize; jCpp++) {

                    int convRowIndex = MOD(iCpe + iCpp, cpe->height);
                    int convColIndex = MOD(jCpe + jCpp, cpe->width);

                    cpe->data[iCpe][jCpe] += image->data[convRowIndex][convColIndex] * kernel->data[iCpp][jCpp];
                }
            }
        }
    }

    return cpe;
}

// Calculates and returns the error image between the input image and the halftone passed.
struct doubleImage* calculateErrorImage(struct doubleImage *inputImage, struct pxm_img *halftone) {

    struct doubleImage *errorImage = (struct doubleImage *) multialloc(sizeof(struct doubleImage), 1, 1);

    errorImage->width = inputImage->width;
    errorImage->height = inputImage->height;
    errorImage->borderSize = 0;
    errorImage->data = (double **) multialloc(sizeof(double), 2, errorImage->height, errorImage->width);

    // Calculate the difference between the input image and the halftone.
    for (int i = 0; i < errorImage[0].height; i++) {
        for (int j = 0; j < errorImage[0].width; j++) {
            errorImage->data[i][j] = inputImage->data[i][j] - (double) halftone->mono[i][j];
        }
    }

    return errorImage;
}

// Generates and returns the auto-correlation of the HVS function.
struct doubleImage* generateCpp(struct doubleImage *psf) {

    struct doubleImage * cpp = (struct doubleImage *) multialloc(sizeof(struct doubleImage), 1, 1);

    cpp->height = 1;
    cpp->width = 1;
    cpp->borderSize = 2 * psf->borderSize;

    cpp->data = (double **) multialloc(sizeof(double), 2, 2 * cpp->borderSize + cpp->height, 2 * cpp->borderSize + cpp->width);

    /* Offset array indexing: Center the 0th, 0th index to be in the middle of the matrix. */
    for (int i = 0; i < 2 * cpp->borderSize + cpp->width; i++) {
        cpp->data[i] += cpp->borderSize;
    }

    cpp->data += cpp->borderSize;

    int size = psf->borderSize;

    // Auto-correlation
    for (int m = -2 * size; m < 2 * size + 1; m++) {
        for (int n = -2 * size; n < 2 * size + 1; n++) {
            cpp->data[m][n] = 0.0;

            for (int i = -size; i <= size; i++) {
                for (int j = -size; j <= size; j++) {

                    if (m - i >= -size && m - i <= size && n - j >= -size && n - j <= size)
                        cpp->data[m][n] += psf->data[m - i][n - j] * psf->data[i][j];

                }
            }
        }
    }

    return cpp;
}

// Generates and returns the Nasaenen Human Visual System Point Spread function.
struct doubleImage* generateHvsFunction(Config *config) {

    struct doubleImage *psf;
    double C, D, L, pi, fs, k, denom;

    C = 0.525;
    D = 3.91;
    L = 11;
    pi = 3.141592654;

    fs = pi * config->scaleFactor / 180;
    k = fs / (C * log(L) + D);

    psf = (struct doubleImage *) multialloc(sizeof(struct doubleImage), 1, 1);
    psf->height = 1;
    psf->width = 1;
    psf->borderSize = config->hvsSpreadSize;
    psf->borderSize = config->hvsSpreadSize;
    psf->data = (double **) multialloc(sizeof(double), 2,
        2 * psf->borderSize + psf->height,
        2 * psf->borderSize + psf->width);

    int size = psf->borderSize;

    /* Offset array indexing: Center the 0th, 0th index to be in the middle of the matrix. */
    for (int i = 0; i < 2 * size + psf->width; i++) {
        psf->data[i] += size;
    }

    psf->data += size;

    // Initialize the psf to 0. Note the negative indices.
    for (int i = -size; i <= size; i++)
        for (int j = -size; j <= size; j++)
            psf->data[i][j] = 0;

    // Populate the psf.
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {

            denom = pow((pow(k, 2.) + 4 * pow(pi, 2.) * (pow((double) i, 2.) + pow((double) j, 2.))), 1.5);
            psf->data[i][j] = 2 * pi * k / denom;
        }
    }

    normalizeImage(psf);

    // Windowing psf with a circular window
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {

            double dist = sqrt(pow((double) i, 2) + pow((double) j, 2));
            if (dist >= size + 1) {
                psf->data[i][j] = 0;
            }
            else if (dist > size) {
                psf->data[i][j] = psf->data[i][j] * (double) (size + 1 - dist);
            }
        }
    }

    // Normalize again!
    normalizeImage(psf);

    return psf;
}

// Normalizes the image by dividing each element by the sum of all the image elements.
// The image indices are expected to be rebased such that the center of the image is indices (0, 0).
void normalizeImage(struct doubleImage *image) {

    double sum = 0;
    int size = image->borderSize;

    // Obtain the sum.
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {

            sum += image->data[i][j];
        }
    }

    // Normalize.
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            image->data[i][j] /= sum;
        }
    }
}

// Removes Gamma correction from the input image that has been already corrected. This must be invoked *before* scaling.
void removeGammaCorrection(struct doubleImage *image, double gamma) {
    
	int gammaLookupTable[256];
    int i;
    int j;

    if (gamma < 0.99 || gamma > 1.01) {

        gammaLookupTable[0] = 0;
        gammaLookupTable[255] = 255;

        for (i = 1; i <= 254; i++) {
            gammaLookupTable[i] = (int) MAX(MIN(255.0*pow((double) i / 255.0, gamma) + 0.5, 255), 0);
        }

        /* Gamma-uncorrect the image */
        for (i = 0; i < image->height; i++) {
            for (j = 0; j < image->width; j++) {
                image->data[i][j] = (uint8_t) gammaLookupTable[(int) image->data[i][j]];
            }
        }
    }
}

// Reads the image from the given path, allocates and returns a double image. This function will also scale
// the values from [0.0 - maxGrayValue], to be [1.0 - 0.0].
struct doubleImage* readDoubleImage(char *imagePath, double maxGrayValue, double gamma) {

    // Verify file exists.
    FILE *file;
    if ((file = fopen(imagePath, "rb")) != NULL) {
        // file exists
        fclose(file);
    }
    else {
        //File not found
        fprintf(stdout, "file %s not found. \n", imagePath);
        exit(-1);
    }

    // Check extension, and if pxm, invoke loading from the pxm file. Else, load from tiff.
    char extension[] = "    ";
    int length = strlen(imagePath);

    for (int i = 0; i < 4; i++) {
        extension[3 - i] = imagePath[length - i - 1];
    }

    // if imagePath is not .tif image, then it is a pxm image
    int isTiff = !strcmp(extension, ".tif");

    struct doubleImage *image;

    if (isTiff) {
        image = readTiffImage(imagePath);
    }
    else {
        image = readPxmImage(imagePath);
    }

	// Apply gamma uncorrection, if gamma != 1.0.
	// This must be done *before* scaling.
	if (gamma != 1.0)
		removeGammaCorrection(image, gamma);

    // Scale values to the [0, 1] range.
    for (int i = 0; i < image->height; i++) {
        for (int j = 0; j < image->width; j++) {
            image->data[i][j] = (maxGrayValue - image->data[i][j]) / maxGrayValue;
        }
    }

    return image;
}

// Reads a tiff image from the given path, converts it into a double image and returns the result.
struct doubleImage* readTiffImage(char* imagePath) {
    FILE *file;
    TIFF_img image_tif;

    // open image file
    if ((file = fopen(imagePath, "rb")) == NULL) {
        fprintf(stderr, "Cannot open file %s. \n", imagePath);
        exit(-1);
    }

    // read image
    if (read_TIFF(imagePath, &image_tif)) {
        fprintf(stderr, "error reading file %s. \n", imagePath);
        exit(-1);
    }
    // close image file
    fclose(file);

    // check the type of image data
    if (image_tif.TIFF_type != 'g') {

        fprintf(stderr, "error: image must be 8-bit graylevel.\n");
        free_TIFF(&image_tif);
        exit(-1);
    }

    // Convert tif to double image.
    struct doubleImage *image = (struct doubleImage*) multialloc(sizeof(struct doubleImage), 1, 3);
    
    image->height = image_tif.height;
    image->width = image_tif.width;

    image->data = (double **) multialloc(sizeof(double), 2, image->height, image->width);

    for (int i = 0; i < image->height; i++) {
        for (int j = 0; j < image->width; j++) {
            image->data[i][j] = image_tif.mono[i][j];
        }
    }

    return image;
}

// Reads a PXM image from the given path, converts it into a double image and returns the result.
struct doubleImage* readPxmImage(char* imagePath) {
    FILE* file;
    struct pxm_img image_pxm;

    file = fopen(imagePath, "rb");
    int status = read_pxm(file, &image_pxm);

    if (status) {
        fprintf(stderr, "Cannot read pxm file %s. \n", imagePath);
        exit(-1);
    }

    fclose(file);

    if (image_pxm.pxm_type == 'p') {
        fprintf(stderr, "Cannot process color images.\n");
        free_pxm(&image_pxm);
        exit(-1);
    }    

    // Convert to double image.
    struct doubleImage *image = (struct doubleImage*) multialloc(sizeof(struct doubleImage), 1, 3);
    
    image->height = image_pxm.height;
    image->width = image_pxm.width;
    image->data = (double **) multialloc(sizeof(double), 2, image->height, image->width);

    for (int i = 0; i < image->height; i++) {
        for (int j = 0; j < image->width; j++) {
            image->data[i][j] = image_pxm.mono[i][j];
        }
    }

    free_pxm(&image_pxm);

    return image;
}

// Saves the image passed in a text file, to be opened in MATLAB.
void saveBasedImageToText(struct doubleImage *image, char *filePath) {

	FILE *fid = fopen(filePath, "wb");

	for (int i = -image->borderSize; i <= image->borderSize; i++) {
		for (int j = -image->borderSize; j <= image->borderSize; j++) {
			fprintf(fid, "%+.15f, ", image->data[i][j]);
		}

		fprintf(fid, "\n");
	}

	fclose(fid);
}

// Saves the image passed in a text file, to be opened in MATLAB.
void saveImageToText(double **data, int height, int width, char *filePath) {

	FILE *fid = fopen(filePath, "wb");

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			fprintf(fid, "%+.15f, ", data[i][j]);
		}

		fprintf(fid, "\n");
	}

	fclose(fid);
}

// Saves the image passed in a text file, to be opened in MATLAB.
void saveHalftoneToText(uint8_t **data, int height, int width, char *filePath) {

	FILE *fid = fopen(filePath, "wb");

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			fprintf(fid, "%1d ", data[i][j]);
		}

		fprintf(fid, "\n");
	}

	fclose(fid);
}
