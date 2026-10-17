#include "../FCTAPI.h"
#include "./D3D12_InputLayout.h"

namespace FCT
{
    namespace RHI
    {
        D3D12_InputLayout::D3D12_InputLayout(D3D12_Context* ctx)
            : m_ctx(ctx)
        {
        }

        void D3D12_InputLayout::splitSemantic(const char* rawSemantic, std::string& outName, UINT& outIndex)
        {
            std::string s = rawSemantic ? rawSemantic : "";
            size_t pos = s.size();
            while (pos > 0 && s[pos - 1] >= '0' && s[pos - 1] <= '9')
            {
                --pos;
            }
            if (pos < s.size() && pos > 0)
            {
                outName = s.substr(0, pos);
                outIndex = static_cast<UINT>(std::stoul(s.substr(pos)));
            }
            else
            {
                outName = s;
                outIndex = 0;
            }
        }

        void D3D12_InputLayout::create()
        {
            m_semanticNames.clear();
            m_inputElements.clear();

            size_t totalElements = 0;
            for (const auto& [slot, layout] : m_vertexLayouts)
            {
                totalElements += layout.getElementCount();
            }
            m_semanticNames.reserve(totalElements);
            m_inputElements.reserve(totalElements);

            for (const auto& [slot, layout] : m_vertexLayouts)
            {
                InputRate rate = InputRate::PerVertex;
                auto rateIt = m_inputRates.find(slot);
                if (rateIt != m_inputRates.end())
                {
                    rate = rateIt->second;
                }

                for (size_t i = 0; i < layout.getElementCount(); ++i)
                {
                    const VertexElement& element = layout.getElement(i);
                    std::string rawSemantic = element.getSemantic() ? element.getSemantic() : "";
                    if (rawSemantic == "SV_VertexID" || rawSemantic == "SV_InstanceID")
                    {
                        continue;
                    }

                    std::string parsedName;
                    UINT parsedIndex = 0;
                    splitSemantic(rawSemantic.c_str(), parsedName, parsedIndex);
                    m_semanticNames.push_back(std::move(parsedName));

                    D3D12_INPUT_ELEMENT_DESC desc{};
                    desc.SemanticName = m_semanticNames.back().c_str();
                    desc.SemanticIndex = parsedIndex;
                    desc.Format = ToDxgiFormat(element.getFormat());
                    desc.InputSlot = slot;
                    desc.AlignedByteOffset = static_cast<UINT>(layout.getElementOffset(i));
                    desc.InputSlotClass = (rate == InputRate::PerInstance)
                        ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA
                        : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                    desc.InstanceDataStepRate = (rate == InputRate::PerInstance) ? 1 : 0;

                    m_inputElements.push_back(desc);
                }
            }
        }

        D3D12_INPUT_LAYOUT_DESC D3D12_InputLayout::getInputLayoutDesc() const
        {
            D3D12_INPUT_LAYOUT_DESC desc{};
            desc.pInputElementDescs = m_inputElements.empty() ? nullptr : m_inputElements.data();
            desc.NumElements = static_cast<UINT>(m_inputElements.size());
            return desc;
        }
    }
}
