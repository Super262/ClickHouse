#include <DataStreams/NativeReader.h>
#include <DataStreams/NativeWriter.h>
#include <Formats/FormatFactory.h>
#include <Processors/Formats/IInputFormat.h>
#include <Processors/Formats/IOutputFormat.h>


namespace DB
{


class NativeInputFormat final : public IInputFormat
{
public:
    NativeInputFormat(ReadBuffer & buf, const Block & header)
        : IInputFormat(header, buf)
        , reader(buf, header) {}

    String getName() const override { return "Native"; }

    void resetParser() override
    {
        reader.resetParser();
    }

    Chunk generate() override
    {
        auto block = reader.read();

        assertBlocksHaveEqualStructure(getPort().getHeader(), block, getName());
        block.checkNumberOfRows();

        size_t num_rows = block.rows();
        return Chunk(block.getColumns(), num_rows);
    }

private:
    NativeReader reader;
};

class NativeOutputFormat final : public IOutputFormat
{
public:
    NativeOutputFormat(WriteBuffer & buf, const Block & header)
        : IOutputFormat(header, buf)
        , writer(buf, 0, header)
    {
    }

    String getName() const override { return "Native"; }

    std::string getContentType() const override
    {
        return writer.getContentType();
    }

protected:
    void consume(Chunk chunk) override
    {
        if (chunk)
        {

            auto block = getPort(PortKind::Main).getHeader();
            block.setColumns(chunk.detachColumns());
            writer.write(block);
        }
    }

private:
    NativeWriter writer;
};

void registerInputFormatNative(FormatFactory & factory)
{
    factory.registerInputFormatProcessor("Native", [](
        ReadBuffer & buf,
        const Block & sample,
        const RowInputFormatParams &,
        const FormatSettings &)
    {
        return std::make_shared<NativeInputFormat>(buf, sample);
    });
}

void registerOutputFormatNative(FormatFactory & factory)
{
    factory.registerOutputFormatProcessor("Native", [](
        WriteBuffer & buf,
        const Block & sample,
        const RowOutputFormatParams &,
        const FormatSettings &)
    {
        return std::make_shared<NativeOutputFormat>(buf, sample);
    });
}

}
