#include "3D/OpenGLBuffers.hpp"
#include "DebugLog.hpp"

namespace Anubis::GL {
	bool initialize() {
		genBuffers = (GenBuffersFunction)sf::Context::getFunction("glGenBuffers");
		bindBuffer = (BindBufferFunction)sf::Context::getFunction("glBindBuffer");
		bufferData = (BufferDataFunction)sf::Context::getFunction("glBufferData");
		deleteBuffers = (DeleteBuffersFunction)sf::Context::getFunction("glDeleteBuffers");

		if (!genBuffers || !bindBuffer || !bufferData || !deleteBuffers) {
			DebugError(L"OpenGL buffer functions are unavailable");
			return false;
		}

		DebugStat(L"OpenGL buffer functions loaded");

		genFramebuffers = (GenFramebuffersFunction)sf::Context::getFunction("glGenFramebuffers");
		bindFramebuffer = (BindFramebufferFunction)sf::Context::getFunction("glBindFramebuffer");
		framebufferTexture2D = (FramebufferTexture2DFunction)sf::Context::getFunction("glFramebufferTexture2D");
		checkFramebufferStatus = (CheckFramebufferStatusFunction)sf::Context::getFunction("glCheckFramebufferStatus");
		deleteFramebuffers = (DeleteFramebuffersFunction)sf::Context::getFunction("glDeleteFramebuffers");

		if (!genFramebuffers || !bindFramebuffer || !framebufferTexture2D || !checkFramebufferStatus || !deleteFramebuffers) {
			DebugError(L"OpenGL framebuffer functions are unavailable");
			return false;
		}

		DebugStat(L"OpenGL framebuffer functions loaded");

		genQueries = (GenQueriesFunction)sf::Context::getFunction("glGenQueries");
		deleteQueries = (DeleteQueriesFunction)sf::Context::getFunction("glDeleteQueries");
		beginQuery = (BeginQueryFunction)sf::Context::getFunction("glBeginQuery");
		endQuery = (EndQueryFunction)sf::Context::getFunction("glEndQuery");
		getQueryObjectuiv = (GetQueryObjectuivFunction)sf::Context::getFunction("glGetQueryObjectuiv");

		if (!genQueries || !deleteQueries || !beginQuery || !endQuery || !getQueryObjectuiv) {
			DebugError(L"OpenGL query functions are unavailable");
			return false;
		}

		DebugStat(L"OpenGL query functions loaded");

		genVertexArrays = (GenVertexArraysFunction)sf::Context::getFunction("glGenVertexArrays");
		bindVertexArray = (BindVertexArrayFunction)sf::Context::getFunction("glBindVertexArray");
		deleteVertexArrays = (DeleteVertexArraysFunction)sf::Context::getFunction("glDeleteVertexArrays");
		enableVertexAttribArray = (EnableVertexAttribArrayFunction)sf::Context::getFunction("glEnableVertexAttribArray");
		vertexAttribPointer = (VertexAttribPointerFunction)sf::Context::getFunction("glVertexAttribPointer");
		vertexAttribIPointer = (VertexAttribIPointerFunction)sf::Context::getFunction("glVertexAttribIPointer");

		if (!genVertexArrays || !bindVertexArray || !deleteVertexArrays || !enableVertexAttribArray || !vertexAttribPointer || !vertexAttribIPointer) {
			DebugError(L"OpenGL vertex array functions are unavailable");
			return false;
		}

		DebugStat(L"OpenGL vertex array functions loaded");

		activeTexture = (ActiveTextureFunction)sf::Context::getFunction("glActiveTexture");
		useProgram = (UseProgramFunction)sf::Context::getFunction("glUseProgram");

		if (!activeTexture || !useProgram) {
			DebugError(L"OpenGL shader binding functions are unavailable");
			return false;
		}

		DebugLog(L"OpenGL shader binding functions loaded");

		getUniformLocation = (GetUniformLocationFunction)sf::Context::getFunction("glGetUniformLocation");
		uniformMatrix4fv = (UniformMatrix4fvFunction)sf::Context::getFunction("glUniformMatrix4fv");
		uniform1i = (Uniform1iFunction)sf::Context::getFunction("glUniform1i");
		uniform3f = (Uniform3fFunction)sf::Context::getFunction("glUniform3f");
		uniform4f = (Uniform4fFunction)sf::Context::getFunction("glUniform4f");

		if (!getUniformLocation || !uniformMatrix4fv || !uniform1i || !uniform3f || !uniform4f) {
			DebugError(L"OpenGL uniform functions are unavailable");
			return false;
		}

		DebugLog(L"OpenGL uniform functions loaded");

		texBuffer = (TexBufferFunction)sf::Context::getFunction("glTexBuffer");

		if (!texBuffer) {
			DebugError(L"OpenGL texture buffer function is unavailable");
			return false;
		}

		DebugLog(L"OpenGL texture buffer function loaded");

		return true;
	}
}