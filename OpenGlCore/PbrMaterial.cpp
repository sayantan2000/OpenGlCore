#include "PbrMaterial.h"
#include <glm/gtc/type_ptr.hpp>

// Material texture units (IBL uses 0..2).
static constexpr int UNIT_ALBEDO = 3;
static constexpr int UNIT_NORMAL = 4;
static constexpr int UNIT_METALLIC = 5;
static constexpr int UNIT_ROUGHNESS = 6;
static constexpr int UNIT_AO = 7;

static void BindMap(Shader& shader, const char* hasName, const char* samplerName,
	GLuint tex, int unit)
{
	GLuint id = shader.ID;
	int has = (tex != 0) ? 1 : 0;
	glUniform1i(glGetUniformLocation(id, hasName), has);
	glUniform1i(glGetUniformLocation(id, samplerName), unit);
	if (has)
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, tex);
	}
}

void PbrMaterial::Bind(Shader& shader) const
{
	GLuint id = shader.ID;
	glUniform3fv(glGetUniformLocation(id, "u_albedo"), 1, glm::value_ptr(albedo));
	glUniform1f(glGetUniformLocation(id, "u_metallic"), metallic);
	glUniform1f(glGetUniformLocation(id, "u_roughness"), roughness);
	glUniform1f(glGetUniformLocation(id, "u_ao"), ao);

	BindMap(shader, "u_hasAlbedoMap", "u_albedoMap", albedoMap, UNIT_ALBEDO);
	BindMap(shader, "u_hasNormalMap", "u_normalMap", normalMap, UNIT_NORMAL);
	BindMap(shader, "u_hasMetallicMap", "u_metallicMap", metallicMap, UNIT_METALLIC);
	BindMap(shader, "u_hasRoughnessMap", "u_roughnessMap", roughnessMap, UNIT_ROUGHNESS);
	BindMap(shader, "u_hasAoMap", "u_aoMap", aoMap, UNIT_AO);
}
