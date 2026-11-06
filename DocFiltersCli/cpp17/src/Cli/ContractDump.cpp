/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ContractDump.h"

#include "ArgumentTable.h"
#include "Runtime/JsonWriter.h"

#include <stdexcept>

namespace Hyland
{
	namespace DocumentFilters
	{
		namespace Cli
		{
			namespace Cli
			{
				namespace
				{
					std::string arityText(ArgumentArity arity)
					{
						switch (arity)
						{
							case ArgumentArity::NoValue:
								return "none";
							case ArgumentArity::SingleValue:
								return "single";
							case ArgumentArity::MultipleValue:
								return "multiple";
						}
						throw std::invalid_argument("unknown ArgumentArity");
					}
				}

				namespace ContractDump
				{
					std::string renderCommand(const std::string& command)
					{
						std::string result = "command " + command + "\n";

						for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
						{
							result += "argument\t";
							result += argument->templateText();
							result += '\t';
							result += arityText(argument->arity());
							result += '\t';
							result += argument->description();
							result += '\n';
						}

						return result;
					}

					std::string render()
					{
						std::string result;
						for (const std::string& command : ArgumentTable::commands())
							result += renderCommand(command);
						return result;
					}

					std::string renderJson()
					{
						Runtime::JsonWriter writer;

						writer.beginArray();
						for (const std::string& command : ArgumentTable::commands())
						{
							writer.beginObject();
							writer.property("command", command);

							writer.name("arguments");
							writer.beginArray();
							for (const std::shared_ptr<const IArgumentDescriptor>& argument : ArgumentTable::describeCommand(command))
							{
								writer.beginObject();
								writer.property("template", argument->templateText());
								writer.property("description", argument->description());
								writer.property("arity", arityText(argument->arity()));
								writer.endObject();
							}
							writer.endArray();

							writer.endObject();
						}
						writer.endArray();

						return writer.toString();
					}
				}
			}
		}
	}
}
