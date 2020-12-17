#ifndef _CAMERA_PASS_
#define _CAMERA_PASS_

#ifndef CAMERA_PASS_REGISTER
#define CAMERA_PASS_REGISTER b1
#endif

cbuffer CameraPass : register(CAMERA_PASS_REGISTER){
    float4x4 view;
    float4x4 perspect;
    float3   cameraPos;
};

#endif //_CAMERA_PASS_