Shader "Unlit/TextureComparisonShader"
{
    Properties
    {
        _MainTex ("Texture 1", 2D) = "white" {}
		_Tex2("Texture 2", 2D) = "white" {}
		_Amplify("Amplify", Float) = 1
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
			sampler2D _Tex2;
            float4 _MainTex_ST;
			float _Amplify;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                // sample the textures
                float4 col1 = tex2D(_MainTex, i.uv);
				float4 col2 = tex2D(_Tex2, i.uv);
				// return the absolute difference between the two textures, amplified by the _Amplify factor
                return saturate(abs(col1-col2) * _Amplify);
            }
            ENDCG
        }
    }
}
