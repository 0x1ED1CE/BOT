--[[
MIT License

Copyright (c) 2025 Dice

Permission is hereby granted, free of charge, to any person obtaining a copy 
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]

local bot={
	version="1.3.2"
}

--Helper functions

bot.values_tostring=function(
	bot,
	values
)
	if values==nil then
		return ""
	end

	local string_=""

	for _,value in ipairs(values) do
		string_=string_..string.char(value)
	end

	return string_
end

bot.report=function(
	bot,
	message,
	source,
	index
)
	local row    = 1
	local column = 0

	for i=1,index do
		if i>#source then
			break
		end

		if source:sub(i,i)=="\n" then
			row    = row+1
			column = 0
		end

		column = column+1
	end

	print(("[%d,%d] %s"):format(
		row,
		column,
		message
	))
end

--Token types

bot.token={
	SYMBOL = 1,
	NUMBER = 2,
	STRING = 3,
	SPLICE = 4,
	INSERT = 5,
	LENGTH = 6,
	MODIFY = 7
}

--Block types

bot.static_blocks={
	["nop"] = string.char(0x00),
	["int"] = string.char(0x01),
	["jmp"] = string.char(0x20),
	["jmc"] = string.char(0x21),
	["ceq"] = string.char(0x22),
	["cne"] = string.char(0x23),
	["cls"] = string.char(0x24),
	["cle"] = string.char(0x25),
	["hop"] = string.char(0x30),
	["pos"] = string.char(0x31),
	["set"] = string.char(0x32),
	["get"] = string.char(0x33),
	["pop"] = string.char(0x34),
	["rot"] = string.char(0x35),
	["add"] = string.char(0x40),
	["sub"] = string.char(0x41),
	["mul"] = string.char(0x42),
	["div"] = string.char(0x43),
	["mod"] = string.char(0x44),
	["min"] = string.char(0x45),
	["not"] = string.char(0x50),
	["and"] = string.char(0x51),
	["bor"] = string.char(0x52),
	["xor"] = string.char(0x53),
	["lsh"] = string.char(0x54),
	["rsh"] = string.char(0x55)
}

bot.dynamic_blocks={
	["rem"] = function(
		bot,
		context,
		arguments
	)
	end,
	["num"] = function(
		bot,
		context,
		arguments
	)
		for _,argument in ipairs(arguments) do
			if #argument==1 and argument[1]==0 then
				context.output[context.o_index] = string.char(0x10)
				context.o_index                 = context.o_index+1
			else
				local bytes = 0
				local index = 0

				--Count total number of bytes
				for i,value in ipairs(argument) do
					value = value&0xFFFFFFFF

					local word = 0

					repeat
						word = word+1
					until 2^(word*8)>value

					bytes = bytes+word
				end

				--Add padding
				if bytes%4~=0 then
					local padding = 4-bytes%4

					context.output[context.o_index] = string.char(0x10+4-padding)
					context.o_index                 = context.o_index+1

					bytes = bytes+padding
					index = padding
				end

				--Pack values per argument
				for _,value in ipairs(argument) do
					value = value&0xFFFFFFFF

					local todo = 0
					local word = math.min(bytes-index,4)

					repeat
						todo = todo+1
					until 2^(todo*8)>value

					todo = math.min(todo,word)

					if index%4==0 then
						context.output[context.o_index] = string.char(0x10+word)
						context.o_index                 = context.o_index+1
					end

					for i=1,todo do
						context.output[context.o_index] = string.char((value>>((todo-i)*8))&0xFF)
						context.o_index                 = context.o_index+1
						index                           = index+1
					end
				end
			end
		end
	end,
	["sec"] = function(
		bot,
		context,
		arguments
	)
		local tag=bot:values_tostring(arguments[1])

		if context.sections[tag] then
			return bot:report(
				"Duplicate section: "..tag,
				context.source,
				context.s_index
			)
		end

		for i=#context.pointers,1,-3 do
			if context.pointers[i-2]==tag then
				local address   = context.o_index-1
				context.o_index = context.pointers[i]

				bot:translate(
					("num %d %d %d %d"):format(
						(address>>24) & 0xFF,
						(address>>16) & 0xFF,
						(address>>8)  & 0xFF,
						(address)     & 0xFF
					),
					context,
					nil
				)

				context.o_index = address+1

				table.remove(context.pointers,i)
				table.remove(context.pointers,i-1)
				table.remove(context.pointers,i-2)

				if tag=="" then
					return
				end
			end
		end

		if tag=="" then
			return bot:report(
				"Missing pointer",
				context.source,
				context.s_index
			)
		end

		context.sections[tag]=context.o_index-1
	end,
	["rec"] = function(
		bot,
		context,
		arguments
	)
		local tag     = bot:values_tostring(arguments[1])
		local address = context.sections[tag]

		if address then
			bot:translate(
				"num "..tostring(address),
				context,
				nil
			)

			return
		end

		context.pointers[#context.pointers+1] = tag
		context.pointers[#context.pointers+1] = context.s_index
		context.pointers[#context.pointers+1] = context.o_index

		bot:translate(
			"num 0 0 0 0",
			context,
			nil
		)
	end,
	["mac"] = function(
		bot,
		context,
		arguments
	)
		local symbol = bot:values_tostring(arguments[1])
		local source = bot:values_tostring(arguments[2])

		context.macros[symbol] = source
	end,
	["gbl"] = function(
		bot,
		context,
		arguments
	)
		local symbol     = bot:values_tostring(arguments[1])
		local allocation = 1

		if arguments[2]~=nil then
			allocation=arguments[2][1]
		end

		for i=#context.globals,1,-3 do
			if context.globals[i-2]==symbol then
				if context.globals[i]<allocation then
					return bot:report(
						"Cannot redefine global size",
						context.source,
						context.s_index
					)
				end

				bot:translate(
					"num "..tostring(context.globals[i-1]),
					context,
					nil
				)

				return
			end
		end

		bot:translate(
			"num "..tostring(context.g_index),
			context,
			nil
		)

		context.globals[#context.globals+1] = symbol
		context.globals[#context.globals+1] = context.g_index
		context.globals[#context.globals+1] = allocation

		context.g_index = context.g_index+allocation
	end
}

--Parsing functions

bot.tokenize=function(
	bot,
	source,
	index_a
)
	while index_a<=#source do
		local char_=source:sub(
			index_a,
			index_a
		)

		if char_:find("[%a_]+") then
			local index_b=index_a
			local next_

			repeat
				index_b = index_b+1
				next_   = source:sub(
					index_b,
					index_b
				)
			until
				index_b>#source or
				not next_:find("[%a_%d]+")

			return
				bot.token.SYMBOL,
				index_a,
				index_b-1
		end

		if char_:find("%d") then
			local index_b=index_a
			local next_

			repeat
				index_b = index_b+1
				next_   = source:sub(
					index_b,
					index_b
				)
			until
				index_b>#source or
				next_:find("%s") or
				next_:find("%c") or
				next_==","

			if source:sub(index_a,index_b-1)~="-" then
				return
					bot.token.NUMBER,
					index_a,
					index_b-1
			end
		end

		if char_=='"' or char_=="'" then
			local index_b=index_a
			local next_

			repeat
				index_b = index_b+1
				next_   = source:sub(
					index_b,
					index_b
				)
			until
				index_b>=#source or
				next_==source:sub(
					index_a,
					index_a
				)

			return
				bot.token.STRING,
				index_a,
				index_b
		end

		if char_=="," then
			return
				bot.token.SPLICE,
				index_a,
				index_a
		end

		if char_=="@" then
			local index_b=index_a
			local next_

			repeat
				index_b = index_b+1
				next_   = source:sub(
					index_b,
					index_b
				)
			until
				index_b>#source or
				next_:find("%s") or
				next_:find("%c") or
				next_==","

			return
				bot.token.INSERT,
				index_a,
				index_b-1
		end

		if char_=="#" then
			local index_b=index_a
			local next_

			repeat
				index_b = index_b+1
				next_   = source:sub(
					index_b,
					index_b
				)
			until
				index_b>#source or
				next_:find("%s") or
				next_:find("%c") or
				next_==","

			return
				bot.token.LENGTH,
				index_a,
				index_b-1
		end

		if (
			char_=="+" or
			char_=="-" or
			char_=="*" or
			char_=="/" or
			char_=="&" or
			char_=="|" or
			char_=="~" or
			char_=="^" or
			char_=="<" or
			char_==">"
		) then
			return
				bot.token.MODIFY,
				index_a,
				index_a
		end

		index_a=index_a+1
	end

	return nil,index_a,index_a
end

bot.parameterize=function(
	bot,
	source,
	context,
	index,
	meta
)
	local arguments = {}

	local token
	local index_a = index
	local index_b = index

	while index_b<=#source do
		token,
		index_a,
		index_b
		=bot:tokenize(
			source,
			index_b+1
		)

		if token==bot.token.SYMBOL then
			break
		elseif token==bot.token.SPLICE then
			arguments[#arguments+1]={}
		elseif token==bot.token.NUMBER then
			local argument=arguments[#arguments]

			if argument==nil then
				argument                = {}
				arguments[#arguments+1] = argument
			end

			argument[#argument+1]=tonumber(
				source:sub(
					index_a,
					index_b
				)
			)
		elseif token==bot.token.STRING then
			local argument=arguments[#arguments]

			if argument==nil then
				argument                = {}
				arguments[#arguments+1] = argument
			end

			for i=index_a+1,index_b-1 do
				argument[#argument+1]=source:byte(i,i)
			end
		elseif token==bot.token.INSERT then
			local argument=arguments[#arguments]

			if argument==nil then
				argument                = {}
				arguments[#arguments+1] = argument
			end

			local id=tonumber(
				source:sub(
					index_a+1,
					index_b
				)
			)+1

			if meta[id] then
				for _,value in ipairs(meta[id]) do
					argument[#argument+1]=value
				end
			end
		elseif token==bot.token.LENGTH then
			local argument=arguments[#arguments]

			if argument==nil then
				argument                = {}
				arguments[#arguments+1] = argument
			end

			local id=tonumber(
				source:sub(
					index_a+1,
					index_b
				)
			)+1

			if meta[id] then
				local bytes=0

				for _,value in ipairs(meta[id]) do
					repeat
						bytes = bytes+1
					until 2^(bytes*8)>=value
				end

				argument[#argument+1]=bytes
			end
		elseif token==bot.token.MODIFY then
			local arg    = arguments[#arguments]
			local effect = source:sub(
				index_a,
				index_b
			);

			if effect=="+" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]+arg[#arg]
				arg[#arg]   = nil
			elseif effect=="-" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]-arg[#arg]
				arg[#arg]   = nil
			elseif effect=="*" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]*arg[#arg]
				arg[#arg]   = nil
			elseif effect=="/" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]//arg[#arg]
				arg[#arg]   = nil
			elseif effect=="&" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]&arg[#arg]
				arg[#arg]   = nil
			elseif effect=="|" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]|arg[#arg]
				arg[#arg]   = nil
			elseif effect=="^" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]~arg[#arg]
				arg[#arg]   = nil
			elseif effect=="<" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]<<arg[#arg]
				arg[#arg]   = nil
			elseif effect==">" and arg~=nil and #arg>=2 then
				arg[#arg-1] = arg[#arg-1]>>arg[#arg]
				arg[#arg]   = nil
			elseif effect=="~" and arg~=nil and #arg>=1 then
				arg[#arg] = ~arg[#arg]
			else
				bot:report(
					"Missing operand for "..effect,
					context.source,
					context.s_index
				)
			end
		end
	end

	return arguments
end

bot.translate=function(
	bot,
	source,
	context,
	meta
)
	local token_type = 0
	local index_a    = 0
	local index_b    = 0

	while index_b<=#source do
		token_type,
		index_a,
		index_b
		=bot:tokenize(
			source,
			index_b+1
		)

		if source==context.source then
			context.s_index=index_a
		end

		if token_type==bot.token.SYMBOL then
			local symbol = source:sub(
				index_a,
				index_b
			)
			local arguments = bot:parameterize(
				source,
				context,
				index_b+1,
				meta
			)

			if bot.static_blocks[symbol] then
				context.output[context.o_index] = bot.static_blocks[symbol]
				context.o_index                 = context.o_index+1
			elseif bot.dynamic_blocks[symbol] then
				bot.dynamic_blocks[symbol](
					bot,
					context,
					arguments
				)
			elseif context.macros[symbol] then
				bot:translate(
					context.macros[symbol],
					context,
					arguments
				)
			else
				bot:report(
					"Unknown symbol: "..symbol,
					context.source,
					context.s_index
				)
			end
		end
	end
end

bot.compile=function(
	bot,
	source,
	meta
)
	local context = {
		source   = source,
		output   = {},
		macros   = {},
		sections = {},
		pointers = {},
		globals  = {},
		s_index  = 1,
		o_index  = 1,
		g_index  = 0
	}

	bot:translate(
		"num 0 0 0 0 hop",
		context,
		nil
	)

	bot:translate(
		source,
		context,
		meta
	)

	context.o_index = 1

	bot:translate(
		("num %d %d %d %d"):format(
			(context.g_index>>24) & 0xFF,
			(context.g_index>>16) & 0xFF,
			(context.g_index>>8)  & 0xFF,
			(context.g_index)     & 0xFF
		),
		context,
		nil
	)

	for i=#context.pointers,1,-3 do
		bot:report(
			"Undefined section: "..context.pointers[i-2],
			context.source,
			context.pointers[i-1]
		)
	end

	return
		table.concat(context.output),
		context
end

--Command line

if #arg==0 then
	print(
		"BOT "..bot.version.." Copyright (C) 2025 Dice\n"..
		"Usage: bot -i <file> -e <file>\n"..
		"Options:\n"..
		"-i\tImport source\n"..
		"-e\tExport binary\n"..
		"-s\tExport symbol"
	)

	return
end

local source = {}
local option

for _,argument in ipairs(arg) do
	if argument:sub(1,1)=="-" then
		option=argument
	elseif not option then
		print("Missing argument")

		return
	elseif option=="-i" then
		local file=io.open(argument,"rb")

		if not file then
			print("Cannot open file: "..argument)

			return
		end

		source[#source+1]=file:read("*a")

		file:close()
	elseif option=="-e" then
		local file=io.open(argument,"wb")

		if not file then
			print("Cannot open file: "..argument)

			return
		end

		local binary = bot:compile(table.concat(source))

		file:write(binary)

		file:close()
	elseif option=="-s" then
		local file=io.open(argument,"w")

		if not file then
			print("Cannot open file: '"..filename.."'")
		end

		local binary,context=bot:compile(table.concat(source))

		for tag,address in pairs(context.sections) do
			file:write(('%.8X sec "%s"\n'):format(address,tag))
		end

		for i=1,#context.globals,3 do
			file:write(('%.8X gbl "%s"\n'):format(
				context.globals[i+1],
				context.globals[i]
			))
		end

		file:close()
	else
		print("Invalid option")

		return
	end
end
