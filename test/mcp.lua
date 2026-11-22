local scoutBinary = arg[1]

assert (scoutBinary ~= nil, "usage: mcp.lua <scout>")

local scriptDirectory = vim.fn.fnamemodify (arg[0], ":h")
local echoPath = scriptDirectory .. "/fixture/echo.md"
local failPath = scriptDirectory .. "/fixture/fail.md"
local missingPath = scriptDirectory .. "/fixture/missing.md"
local baselinePath = scriptDirectory .. "/baseline/mcp.txt"

local buffer = ""
local lines = {}
local firstByte = nil

local handle = vim.system ({ scoutBinary, "--mcp" }, {
    stdin = true,
    text = true,
    stdout = function (_, data)
        if data ~= nil then
            if firstByte == nil and #data > 0 then
                firstByte = data:sub (1, 1)
            end

            buffer = buffer .. data

            while true do
                local newlineIndex = buffer:find ("\n", 1, true)

                if newlineIndex == nil then
                    break
                end

                local line = buffer:sub (1, newlineIndex - 1):gsub ("\r$", "")
                buffer = buffer:sub (newlineIndex + 1)

                if #line > 0 then
                    lines[#lines + 1] = line
                end
            end
        end
    end,
})

local function readResponse (timeoutMs)
    local ready = vim.wait (timeoutMs, function ()
        return #lines > 0
    end, 10)

    assert (ready and #lines > 0, "timed out waiting for response")
    assert (firstByte == "{", "unexpected byte before first JSON line")

    local line = table.remove (lines, 1)
    assert (line:sub (1, 1) == "{", "non-JSON stdout line: " .. line)

    return vim.json.decode (line)
end

local function sendRequest (id, method, params)
    local payload = vim.json.encode ({
        jsonrpc = "2.0",
        id = id,
        method = method,
        params = params,
    })

    handle:write (payload .. "\n")
    return readResponse (60000)
end

local meta = {
    ["io.modelcontextprotocol/protocolVersion"] = "2026-07-28",
    ["io.modelcontextprotocol/clientCapabilities"] = vim.empty_dict (),
}

local transcript = {}

local discover = sendRequest (1, "server/discover", { _meta = meta })
transcript[#transcript + 1] = "server/discover result supportedVersions=" .. vim.json.encode (discover.result.supportedVersions)

local list = sendRequest (2, "tools/list", { _meta = meta })
transcript[#transcript + 1] = "tools/list result count=" .. tostring (#list.result.tools)

local echoRes = sendRequest (3, "tools/call", {
    _meta = meta,
    name = "run",
    arguments = { manifest = echoPath },
})
assert (echoRes.result.isError ~= true, "expected echo run to succeed")
assert (echoRes.result.structuredContent.exitCode == 0, "expected exitCode 0")
assert (echoRes.result.structuredContent.output == "probe\n", "expected output probe\\n")
transcript[#transcript + 1] = "tools/call run echo ok exitCode=0 output=probe"

local failRes = sendRequest (4, "tools/call", {
    _meta = meta,
    name = "run",
    arguments = { manifest = failPath },
})
assert (failRes.result.isError == true, "expected fail run to error")
assert (failRes.result.content[1].text:find ("toolchain command failed", 1, true) ~= nil, "expected toolchain command failed")
transcript[#transcript + 1] = "tools/call run fail isError=true toolchain command failed"

local relativeRes = sendRequest (5, "tools/call", {
    _meta = meta,
    name = "run",
    arguments = { manifest = "echo.md" },
})
assert (relativeRes.result.isError == true, "expected relative path to error")
assert (relativeRes.result.content[1].text == "manifest is not an absolute path", "expected manifest is not an absolute path")
transcript[#transcript + 1] = "tools/call run relative isError=true manifest is not an absolute path"

local missingRes = sendRequest (6, "tools/call", {
    _meta = meta,
    name = "run",
    arguments = { manifest = missingPath },
})
assert (missingRes.result.isError == true, "expected missing manifest to error")
assert (missingRes.result.content[1].text == "not found: missing.md", "expected not found: missing.md")
transcript[#transcript + 1] = "tools/call run missing isError=true not found: missing.md"

handle:write (nil)
local exited = handle:wait (10000)
transcript[#transcript + 1] = "exit " .. tostring (exited.code)

local baselineFile = assert (io.open (baselinePath, "r"))
local expected = {}

for line in baselineFile:lines () do
    expected[#expected + 1] = line:gsub ("\r$", "")
end

baselineFile:close ()

local matched = (#expected == #transcript)

if matched then
    for index = 1, #expected do
        if expected[index] ~= transcript[index] then
            matched = false
            break
        end
    end
end

if matched then
    os.exit (0)
else
    io.stderr:write ("FAIL: transcript mismatch against " .. baselinePath .. "\n")

    local maxLines = math.max (#expected, #transcript)

    for index = 1, maxLines do
        local expectedLine = expected[index] or "<missing>"
        local actualLine = transcript[index] or "<missing>"

        if expectedLine ~= actualLine then
            io.stderr:write ("line " .. index .. ":\n")
            io.stderr:write ("  expected: " .. expectedLine .. "\n")
            io.stderr:write ("  actual:   " .. actualLine .. "\n")
        end
    end

    os.exit (1)
end
