#version 150 core
#define MAX_VALUE 32768

uniform sampler2D vDatacost;
uniform sampler2D vSmoothcost;
uniform sampler2D vTexCamera;
uniform sampler2D vTexFBO;

uniform vec2 vResolution;

in vec2 finalTexCoord;

uniform int vPass;

layout(location=0) out vec4 firstFrag;	// Terminals
layout(location=1) out vec4 secondFrag; // right
layout(location=2) out vec4 thirdFrag;	// down

void main(void)
{
	if(vPass == 0)
	{
		vec4 lCameraTex = texture(vTexCamera, finalTexCoord.st);
		vec4 lFBOTex = texture(vTexFBO, finalTexCoord.st);
		firstFrag = lCameraTex;
	}
	else if(vPass == 1)
	{
		// Discrétisation des données
		// vDatacost sur un seul canal
		vec4 lDataTex = texture(vDatacost, finalTexCoord.st);
		// vSmoothcost sur deux canaux
		vec4 lSmoothTex = texture(vSmoothcost, finalTexCoord.st);

		// Calcul de la position dans la texture en coordonnées *réelles*
		float lX = finalTexCoord.s*vResolution.s*2.f;
		float lY = finalTexCoord.t*vResolution.t*2.f;
		
		// Vérifie si nous sommes sur une ligne et une colonne paires
		bool lEvenLine, lEvenColumn;
		if(mod(floor(lX), 2) == 0)
			lEvenColumn = true;
		else
			lEvenColumn = false;

		if(mod(floor(lY), 2) == 0)
			lEvenLine = true;
		else
			lEvenLine = false;

		// Matrice des coûts pour les connexions aux terminaux
		// Colonnes et lignes paires
		if(lEvenColumn == true && lEvenLine == true)
		{
			firstFrag.r = lDataTex.r + 0.5f - lDataTex.g;
		}
		// Sur le reste du graphe ... tout est nul !
		else if(lEvenColumn == false && lEvenLine == true)
		{
			firstFrag.r = 0.f;
		}

		// Matrices des coûts de lissage
		// Colonnes et lignes paires
		if(lEvenColumn == true && lEvenLine == true)
		{
			// Si on n'est pas sur le premier pixel sur x, ni sur le dernier
			if(lX > 1 && lX < vResolution.s*2-1)
				secondFrag.r = lSmoothTex.r;
			else
				secondFrag.r = 0.f;

			// Si on n'est pas sur le premier pixel sur y
			if(lY > 1 && lY < vResolution.t*2-1)
				thirdFrag.r = lSmoothTex.g;
			else
				thirdFrag.r = 0.f;
		}
		// colonnes impaires et ligne paire
		else if(lEvenColumn == false && lEvenLine == true)
		{
			if(lX > 1 && lX < vResolution.s*2-1)
				secondFrag.r = lSmoothTex.r;
			else
				secondFrag.r = 0.f;

			thirdFrag.r = 0.f;
		}
		// colonnes paires et lignes impaires
		else if(lEvenColumn == true && lEvenLine == false)
		{
			secondFrag.r = 0.f;
			
			if(lY > 1 && lY < vResolution.s*2-1)
				thirdFrag.r = lSmoothTex.g;
			else
				thirdFrag.r = 0.f;
		}
		// colonnes impaires et lignes impaires
		else
		{
			secondFrag.r = 0.f;
			thirdFrag.r = 0.f;
		}	
	}
}
