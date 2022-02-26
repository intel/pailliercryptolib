
#include <stdio.h>
#include "he_qat_context.h"

int main()
{
    HE_QAT_STATUS status = HE_QAT_STATUS_FAIL;

    status = acquire_qat_devices();
    if (HE_QAT_STATUS_SUCCESS == status) {
        printf("Completed acquire_qat_devices() successfully.\n");
    } else { 
        printf("acquire_qat_devices() failed.\n");
        exit(1);
    }

    status = release_qat_devices();
    if (HE_QAT_STATUS_SUCCESS == status) {
        printf("Completed release_qat_devices() successfully.\n");
    } else { 
        printf("release_qat_devices() failed.\n");
        exit(1);
    } 
    
    return 0;
}
