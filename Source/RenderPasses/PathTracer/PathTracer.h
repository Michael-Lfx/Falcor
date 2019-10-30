/***************************************************************************
# Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "Falcor.h"
#include "FalcorExperimental.h"
#include "Utils/Sampling/SampleGenerator.h"
#include "Utils/Debug/PixelDebug.h"
#include "Experimental/Scene/Lights/EnvProbe.h"
#include "Experimental/Scene/Lights/EmissiveUniformSampler.h"
#include "RenderGraph/RenderPassHelpers.h"
#include "PathTracerParams.h"
#include "Logging.h"

using namespace Falcor;

/** Base class for path tracers.
*/
class PathTracer : public RenderPass, inherit_shared_from_this<RenderPass, PathTracer>
{
public:
    using SharedPtr = std::shared_ptr<PathTracer>;

    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pRenderContext, const CompileData& compileData) override;
    virtual void setScene(RenderContext* pRenderContext, const Scene::SharedPtr& pScene) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override;
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override { return false; }

protected:
    PathTracer() = default;

    virtual bool init(const Dictionary& dict);
    virtual void recreateVars() {}

    void validateParameters();
    bool initLights(RenderContext* pRenderContext);
    bool updateLights(RenderContext* pRenderContext);
    uint32_t maxRaysPerPixel() const;
    bool beginFrame(RenderContext* pRenderContext, const RenderData& renderData);
    void endFrame(RenderContext* pRenderContext, const RenderData& renderData);
    bool renderSamplingUI(Gui::Widgets& widget);
    bool renderLightsUI(Gui::Widgets& widget);
    void renderLoggingUI(Gui::Widgets& widget);
    void setStaticParams(ProgramBase* pProgram) const;

    // Constants used in derived classes
    static const std::string kViewDirInput;
    static const std::string kAlbedoOutput;
    static const ChannelList kInputChannels;
    static const ChannelList kOutputChannels;

    // Internal state
    Scene::SharedPtr                    mpScene;                        ///< Current scene.

    SampleGenerator::SharedPtr          mpSampleGenerator;              ///< GPU sample generator.
    EmissiveLightSampler::SharedPtr     mpEmissiveSampler;              ///< Emissive light sampler or nullptr if disabled.
    EnvProbe::SharedPtr                 mpEnvProbe;                     ///< Environment map sampling (if used).
    std::string                         mEnvProbeFilename;              ///< Name of loaded environment map (stripped of full path).

    Logging::SharedPtr                  mStatsLogger;                   ///< Helper for collecting runtime tracing stats.
    PixelDebug::SharedPtr               mPixelDebugger;                 ///< Utility class for pixel debugging (print in shaders).

    // Configuration
    PathTracerParams                    mSharedParams;                  ///< Host/device shared rendering parameters.
    uint32_t                            mSelectedSampleGenerator = SAMPLE_GENERATOR_UNIFORM;              ///< Which pseudorandom sample generator to use.
    EmissiveLightSamplerType            mSelectedEmissiveSampler = EmissiveLightSamplerType::Uniform;     ///< Which emissive light sampler to use.

    EmissiveUniformSampler::EmissiveUniformSamplerOptions   mUniformSamplerOptions;     ///< Current options for the uniform sampler.

    // Runtime data
    bool                                mOptionsChanged = false;        ///< True if the config has changed since last frame.
    bool                                mUseAnalyticLights = false;     ///< True if analytic lights should be used for the current frame. See compile-time constant in StaticParams.slang.
    bool                                mUseEnvLight = false;           ///< True if env map light should be used for the current frame. See compile-time constant in StaticParams.slang.
    bool                                mUseEmissiveLights = false;     ///< True if emissive lights should be taken into account. See compile-time constant in StaticParams.slang.
    bool                                mUseEmissiveSampler = false;    ///< True if emissive light sampler should be used for the current frame. See compile-time constant in StaticParams.slang.
    uint32_t                            mMaxRaysPerPixel = 0;           ///< Maximum number of rays per pixel that will be traced. This is computed based on the current configuration.

    // Scripting
#define serialize(var) \
    if constexpr (!loadFromDict) dict[#var] = var; \
    else if (dict.keyExists(#var)) { if constexpr (std::is_same<decltype(var), std::string>::value) var = (const std::string &)dict[#var]; else var = dict[#var]; vars.emplace(#var); }

    template<bool loadFromDict, typename DictType>
    void serializePass(DictType& dict)
    {
        std::unordered_set<std::string> vars;

        // Add variables here that should be serialized to/from the dictionary.
        serialize(mSharedParams);
        serialize(mSelectedSampleGenerator);
        serialize(mSelectedEmissiveSampler);
        serialize(mUniformSamplerOptions);

        if constexpr (loadFromDict)
        {
            for (const auto& v : dict)
            {
                if (vars.find(v.key()) == vars.end()) logWarning("Unknown field `" + v.key() + "` in a PathTracer dictionary");
            }
        }
    }
#undef serialize
};