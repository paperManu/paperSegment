#version 330 core

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
		vec4 lDataTexRight = textureOffset(vDatacost, finalTexCoord.st, ivec2(2,0));
		vec4 lDataTexDown = textureOffset(vDatacost, finalTexCoord.st, ivec2(0,2));
		// vSmoothcost sur deux canaux
		vec4 lSmoothTex = texture(vSmoothcost, finalTexCoord.st);

		// Récupère la valeur du masque
		float lFGCost, lBGCost;
		bool lFGMask, lBGMask;

		if(lDataTex.r > 0.5f)
		{
			lFGCost = 1.f-lDataTex.r;
			lFGMask = true;
		}
		else
		{
			lFGCost = lDataTex.r;
			lFGMask = false;
		}

		if(lDataTex.g > 0.5f)
		{
			lBGCost = 1.f-lDataTex.g;
			lBGMask = true;
		}
		else
		{
			lBGCost = lDataTex.g;
			lBGMask = false;
		}
		
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
		// A notez qu'on décale tout de 0.5f (soit 32767) puisque la texture
		// de sortie est en 16U (pas de valeurs négatives autorisées)
		// On considère que ce qui est inconnu est dans le BG
		// Colonnes et lignes paires
		if(lEvenColumn == true && lEvenLine == true)
		{
			if(lFGMask == true)
			{
				firstFrag.r = 0.f;
			}
            else if(lBGMask == true)
            {
                firstFrag.r = 1.f;
            }
			else
			{
				firstFrag.r = lFGCost + 0.5f - lBGCost;
			}
		}
		else if(lEvenColumn == false && lEvenLine == true)
		{
			// On vérifie l'appartenance du pixel de droite
			bool lMask = false;
			if(lDataTexRight.r > 0.5f)
				lMask = true;

			if((lFGMask == true && lMask == false) || (lFGMask == false && lMask == true))	
				firstFrag.r = 0.5f - lSmoothTex.r;
			else
				firstFrag.r = 0.5f;
		}
		else if(lEvenColumn == true && lEvenLine == false)
		{
			// On vérifie l'appartenance du pixel du bas
			bool lMask = false;
			if(lDataTexDown.r > 0.5f)
				lMask = true;
			
			if((lFGMask == true && lMask == false) || (lFGMask == false && lMask == true))
				firstFrag.r = 0.5f - lSmoothTex.g;
			else
				firstFrag.r = 0.5f;
		}
		else
		{
			firstFrag.r = 0.5f;
		}

		// Matrices des coûts de lissage
		// Colonnes et lignes paires
		if(lEvenColumn == true && lEvenLine == true)
		{
			// Si on n'est pas sur le premier pixel sur x, ni sur le dernier
			// Si le pixel de droite a le même label
			if(lX > 2 && lX < vResolution.s*2-2)
			{
				secondFrag.r = lSmoothTex.r;
			}
			else
				secondFrag.r = 0.f;

			// Si on n'est pas sur le premier pixel sur y
			if(lY > 2 && lY < vResolution.t*2-2)
			{
				thirdFrag.r = lSmoothTex.g;
			}
			else
				thirdFrag.r = 0.f;
		}
		// colonnes impaires et ligne paire
		else if(lEvenColumn == false && lEvenLine == true)
		{
			if(lX > 2 && lX < vResolution.s*2-2)
			{
				secondFrag.r = lSmoothTex.r;
			}
			else
				secondFrag.r = 0.f;

			thirdFrag.r = 0.f;
		}
		// colonnes paires et lignes impaires
		else if(lEvenColumn == true && lEvenLine == false)
		{
			secondFrag.r = 0.f;

			if(lY > 2 && lY < vResolution.s*2-2)
			{
				thirdFrag.r = lSmoothTex.g;	
			}	
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
