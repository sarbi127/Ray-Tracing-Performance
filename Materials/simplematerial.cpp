#include "simplematerial.hpp"

qbRT::SimpleMaterial::SimpleMaterial()
{

}

qbRT::SimpleMaterial::~SimpleMaterial()
{

}

// Function to return the color.
qbVector<double> qbRT::SimpleMaterial::ComputeColor(	const std::vector<std::shared_ptr<qbRT::ObjectBase>> &objectList,
																											const std::vector<std::shared_ptr<qbRT::LightBase>> &lightList,
																											const std::shared_ptr<qbRT::ObjectBase> &currentObject,
																											const qbVector<double> &intPoint, const qbVector<double> &localNormal,
																											const qbRT::Ray &cameraRay)
{
	// Define the initial material colors.
	qbVector<double> matColor	{3};
	qbVector<double> refColor {3};
	qbVector<double> difColor	{3};
	qbVector<double> spcColor	{3};
	
	// *** Apply any normals maps that may have been assigned.
	qbVector<double> newNormal = localNormal;
	if (m_hasNormalMap)
	{
		qbVector<double> upVector = std::vector<double> {0.0, 0.0, -1.0};
		newNormal = PerturbNormal(newNormal, currentObject -> m_uvCoords, upVector);
	}
	
	// *** Store the current local normal, in case it is needed elsewhere.
	m_localNormal = newNormal;	
	
	/* Note the change of localNormal to newNormal wherever the normal is used
		in the code below. */
	
	// Compute the diffuse component.
	if (!m_hasTexture)
	{
		//difColor = ComputeDiffuseColor(objectList, lightList, currentObject, intPoint, newNormal, m_baseColor);
		difColor = ComputeSpecAndDiffuse(objectList, lightList, currentObject, intPoint, newNormal, m_baseColor, cameraRay);
	}
	else
	{
		//difColor = ComputeDiffuseColor(objectList, lightList, currentObject, intPoint, newNormal, GetTextureColor(currentObject->m_uvCoords));
		qbVector<double> textureColor = GetTextureColor(currentObject->m_uvCoords);
		difColor = ComputeSpecAndDiffuse(objectList, lightList, currentObject, intPoint, newNormal, textureColor, cameraRay);		
	}
	
	// Compute the reflection component.
	if (m_reflectivity > 0.0)
		refColor = ComputeReflectionColor(objectList, lightList, currentObject, intPoint, newNormal, cameraRay);
		
	// Combine reflection and diffuse components.
	matColor = (refColor * m_reflectivity) + (difColor * (1 - m_reflectivity));
	
	/*
	// Compute the specular component.
	if (m_shininess > 0.0)
		spcColor = ComputeSpecular(objectList, lightList, intPoint, newNormal, cameraRay);
		
	// Add the specular component to the final color.
	matColor = matColor + spcColor;
	*/
	
	return matColor;
}

// Function to compute the specular highlights.
qbVector<double> qbRT::SimpleMaterial::ComputeSpecular(	const std::vector<std::shared_ptr<qbRT::ObjectBase>> &objectList,
																												const std::vector<std::shared_ptr<qbRT::LightBase>> &lightList,
																												const qbVector<double> &intPoint, const qbVector<double> &localNormal,
																												const qbRT::Ray &cameraRay)
{
	qbVector<double> spcColor	{3};
	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;
	
	// Loop through all of the lights in the scene.
	for (auto currentLight : lightList)
	{
		/* Check for intersections with all objects in the scene. */
		double intensity = 0.0;
		
		// Construct a vector pointing from the intersection point to the light.
		qbVector<double> lightDir = (currentLight->m_location - intPoint).Normalized();
		
		// Compute a start point.
		qbVector<double> startPoint = intPoint + (lightDir * 0.001);
		
		// Construct a ray from the point of intersection to the light.
		qbRT::Ray lightRay (startPoint, startPoint + lightDir);
		
		/* Loop through all objects in the scene to check if any
			obstruct light from this source. */
		//qbVector<double> poi				{3};
		//qbVector<double> poiNormal	{3};
		//qbVector<double> poiColor		{3};
		qbRT::DATA::hitData hitData;
		bool validInt = false;
		for (auto sceneObject : objectList)
		{
			validInt = sceneObject -> TestIntersection(lightRay, hitData);
			if (validInt)
				break;
		}
		
		/* If no intersections were found, then proceed with
			computing the specular component. */
		if (!validInt)
		{
			// Compute the reflection vector.
			qbVector<double> d = lightRay.m_lab;
			qbVector<double> r = d - (2 * qbVector<double>::dot(d, localNormal) * localNormal);
			r.Normalize();
			
			// Compute the dot product.
			qbVector<double> v = cameraRay.m_lab;
			v.Normalize();
			double dotProduct = qbVector<double>::dot(r, v);
			
			// Only proceed if the dot product is positive.
			if (dotProduct > 0.0)
			{
				intensity = m_reflectivity * std::pow(dotProduct, m_shininess);
			}
		}
		
		red += currentLight->m_color.GetElement(0) * intensity;
		green += currentLight->m_color.GetElement(1) * intensity;
		blue += currentLight->m_color.GetElement(2) * intensity;
	}
	
	spcColor.SetElement(0, red);
	spcColor.SetElement(1, green);
	spcColor.SetElement(2, blue);
	return spcColor;
}
