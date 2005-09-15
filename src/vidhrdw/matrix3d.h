void matrix3d_Multiply( float A[4][4], float B[4][4] );
void matrix3d_Identity( float M[4][4] );
void matrix3d_Translate( float M[4][4], float x, float y, float z );
void matrix3d_RotX( float M[4][4], float thx_sin, float thx_cos );
void matrix3d_RotY( float M[4][4], float thy_sin, float thy_cos );
void matrix3d_RotZ( float M[4][4], float thz_sin, float thz_cos );
