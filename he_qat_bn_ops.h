#pragma once

// One for each consumer
typedef struct {
    HE_QAT_STATUS request_status;
    CpaStatus op_status;
    CpaCyLnModExpOpData op_data;
    CpaFlatBuffer op_output;
    sem_t callback;

} HE_QAT_ModExpRequest;


/// @brief 
/// @function 
/// Callback function for lnModExpPerformOp. It performs any data processing 
/// required after the modular exponentiation.
void lnModExpCallback(void *pCallbackTag,
                          CpaStatus status,
                          void *pOpData,
                          CpaFlatBuffer *pOut);

/// @brief 
/// @function 
/// Perform large number modular exponentiation.
/// @param[in] cyInstHandle QAT instance handle.
CpaStatus lnModExpPerformOp(CpaInstanceHandle cyInstHandle);
//CpaStatus lnModExpPerformOp(QATOpRequestBuffer *buffer);


CpaStatus bnModExpPerformOp(BIGNUM *r, BIGNUM *b, BIGNUM *e, BIGNUM *m, int nbits);

