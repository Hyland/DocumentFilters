using Xunit;
using Amazon.Lambda.TestUtilities;
using Amazon.Lambda.S3Events;

namespace DocFilters.Lambda.Tests;

public class FunctionTest
{
    [Fact]
    public void TestToUpperFunction()
    {

        // Invoke the lambda function and confirm the string was upper cased.
        var function = new S3ConvertToPDF();
        var context = new TestLambdaContext();
        var evnt = new S3Event();
        var tsk = function.FunctionHandler(evnt, context);

        System.Console.WriteLine(tsk.Result);
    }
}