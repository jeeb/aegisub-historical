﻿--[[
 Copyright (c) 2007, Niels Martin Hansen, Rodrigo Braz Monteiro
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   * Neither the name of the Aegisub Group nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
]]

-- Aegisub Automation 4 Lua karaoke templater tool
-- Parse and apply a karaoke effect written in ASS karaoke template language
-- See help file and wiki for more information on this

script_name = "Karaoke Templater"
script_description = "Macro and export filter to apply karaoke effects using the template language"
script_author = "Niels Martin Hansen"
script_version = 1


include("karaskel.lua")


-- Find and parse/prepare all karaoke template lines
function parse_templates(meta, styles, subs)
	local templates = { once = {}, line = {}, syl = {}, char = {}, furi = {}, styles = {} }
	local i = 1
	while i <= #subs do
		aegisub.progress.set((i-1) / #subs * 100)
		local l = subs[i]
		i = i + 1
		if l.class == "dialogue" and l.comment then
			local fx, mods = string.headtail(l.effect)
			fx = fx:lower()
			if fx == "code" then
				parse_code(meta, styles, l, templates, mods)
			elseif fx == "template" then
				parse_template(meta, styles, l, templates, mods)
			end
			templates.styles[l.style] = true
		elseif l.class == "dialogue" and l.effect == "fx" then
			-- this is a previously generated effect line, remove it
			i = i - 1
			subs.delete(i)
		end
	end
	aegisub.progress.set(100)
	return templates
end

function parse_code(meta, styles, line, templates, mods)
	local template = {
		code = line.text,
		loops = 1,
		style = line.style
	}
	local inserted = false

	local rest = mods
	while rest ~= "" do
		local m, t = string.headtail(rest)
		rest = t
		m = m:lower()
		if m == "once" then
			table.insert(templates.once, template)
			inserted = true
		elseif m == "line" then
			table.insert(templates.line, template)
			inserted = true
		elseif m == "syl" then
			table.insert(templates.syl, template)
			inserted = true
		elseif m == "char" then
			table.insert(templates.char, template)
			inserted = true
		elseif m == "furi" then
			table.insert(templates.furi, template)
			inserted = true
		elseif m == "all" then
			template.style = nil
		elseif m == "repeat" or m == "loop" then
			local times, t = string.headtail(rest)
			template.loops = tonumber(times)
			if not template.loops then
				aegisub.out(3, "Failed reading this repeat-count to a number: %s\nIn template code line: %s\nEffect field: %s\n\n", times, line.text, line.effect)
				template.loops = 1
			else
				rest = t
			end
		else
			aegisub.out(3, "Unknown modifier in code template: %s\nIn template code line: %s\nEffect field: %s\n\n", m, line.text, line.effect)
		end
	end
	
	if not inserted then
		table.insert(templates.once, template)
	end
end

template_modifiers = {
	"pre-line", "line", "syl", "char", "furi",
	"all", "repeat", "loop", "notext", "keeptags", "multi", "fx"
}

function parse_template(meta, styles, line, templates, mods)
	local template = {
		t = "",
		pre = "",
		style = line.style,
		loops = 1,
		addtext = true,
		keeptags = false,
		inline_fx = nil,
		multi = false,
		isline = false
	}
	local inserted = false
	
	local rest = mods
	while rest ~= "" do
		local m, t = string.headtail(rest)
		rest = t
		m = m:lower()
		if (m == "pre-line" or m == "line") and not inserted then
			-- should really fail if already inserted
			local id, t = string.headtail(rest)
			id = id:lower()
			-- check that it really is an identifier and not a keyword
			for _, kw in pairs(template_modifiers) do
				if id == kw then
					id = nil
					break
				end
			end
			if id then
				rest = t
			end
			-- get old template if there is one
			if id and templates.line[id] then
				template = templates.line[id]
			elseif id then
				template.id = id
				templates.line[id] = template
			else
				table.insert(templates.line, template)
			end
			inserted = true
			template.isline = true
			-- apply text to correct string
			if m == "line" then
				template.t = template.t .. line.text
			else -- must be pre-line
				template.pre = template.pre .. line.text
			end
		elseif m == "syl" and not template.isline then
			table.insert(templates.syl, template)
			inserted = true
		elseif m == "char" and not template.isline then
			table.insert(templates.char, template)
			inserted = true
		elseif m == "furi" and not template.isline then
			aegisub.debug.out(3, "Warning, furi template class used but furigana support isn't implemented yet\n\n")
			table.insert(templates.furi, template)
			inserted = true
		elseif (m == "pre-line" or m == "line") and inserted then
			aegisub.out(2, "Unable to combine %s class templates with other template classes\n\n", m)
		elseif (m == "syl" or m == "char" or m == "furi") and template.isline then
			aegisub.out(2, "Unable to combine %s class template lines with line or pre-line classes\n\n", m)
		elseif m == "all" then
			template.style = nil
		elseif m == "repeat" or m == "loop" then
			local times, t = string.headtail(rest)
			template.loops = tonumber(times)
			if not template.loops then
				aegisub.out(3, "Failed reading this repeat-count to a number: %s\nIn template line: %s\nEffect field: %s\n\n", times, line.text, line.effect)
				template.loops = 1
			else
				rest = t
			end
		elseif m == "notext" then
			template.addtext = false
		elseif m == "keeptags" then
			template.keeptags = true
		elseif m == "multi" then
			template.multi = true
		elseif m == "fx" then
			local fx, t = string.headtail(rest)
			if fx ~= "" then
				template.fx = fx
				rest = t
			else
				aegisub.out(3, "No fx name following fx modifier\nIn template line: %s\nEffect field: %s\n\n", line.text, line.effect)
				template.fx = nil
			end
		else
			aegisub.out(3, "Unknown modifier in template: %s\nIn template line: %s\nEffect field: %s\n\n", m, line.text, line.effect)
		end
	end
	
	if not inserted then
		table.insert(templates.syl, template)
	end
end

-- Iterator function, return all templates that apply to the given line
function matching_templates(templates, line)
	local lastkey = 0
	local function test_next()
		lastkey = lastkey + 1
		local t = templates[lastkey]
		if not t then
			return nil
		elseif t.style == line.style or not t.style then
			return t
		else
			return test_next()
		end
	end
	return test_next
end


-- Apply the templates
function apply_templates(meta, styles, subs, templates)
	-- the environment the templates will run in
	local tenv = {
		-- put in some standard libs
		string = string,
		math = math
	}
	
	-- run all run-once code snippets
	for k, t in pairs(templates.once) do
		assert(t.code, "WTF, a 'once' template without code?")
		run_template_code(t, tenv)
	end
	
	-- start processing lines
	local i, n = 1, #subs
	while i < n do
		local l = subs[i]
		i = i + 1
		if l.class == "dialogue" and ((l.effect == "" and not l.comment) or (l.effect == "karaoke" and l.comment)) then
			-- make a karaoke source line off it
			l.comment = true
			l.effect = "karaoke"
			subs[i] = l
			l.i = i
			-- and then run it through the templates
			apply_line(meta, styles, subs, l, templates, tenv)
		end
	end
end

function apply_line(meta, styles, subs, line, templates, tenv)
	-- Tell whether any templates were applied to this line, needed to know whether the original line should be removed from input
	local applied_templates = false
	
	-- General variable replacement context
	local varctx = {
		layer = line.layer,
		lstart = line.start_time,
		lend = line.end_time,
		ldur = line.duration,
		lmid = line.start_time + line.duration/2,
		style = line.style,
		actor = line.actor,
		margin_l = ((line.margin_l > 0) and line.margin_l) or line.styleref.margin_l,
		margin_r = ((line.margin_r > 0) and line.margin_r) or line.styleref.margin_r,
		margin_t = ((line.margin_t > 0) and line.margin_t) or line.styleref.margin_t,
		margin_b = ((line.margin_b > 0) and line.margin_b) or line.styleref.margin_b,
		margin_v = ((line.margin_t > 0) and line.margin_t) or line.styleref.margin_t,
		syln = line.karaoke.n,
		li = line.i,
		lleft = line.left,
		lcenter = line.left + line.width/2,
		lright = line.left + line.width,
		lx = line.x,
		ly = line.y
		-- TODO: more positioning vars
	}
	
	-- Specific for whole-line processing
	varctx["start"] = varctx.lstart
	varctx["end"] = varctx.lend
	varctx.dur = varctx.ldur
	varctx.mid = varctx.lmid
	varctx.i = varctx.li
	varctx.left = varctx.lleft
	varctx.center = varctx.lcenter
	varctx.right = varctx.lright
	varctx.x = varctx.lx
	varctx.y = varctx.ly
	
	local function set_ctx_syl(syl)
		varctx.sstart = syl.start_time
		varctx.send = syl.end_time
		varctx.sdur = syl.duration
		varctx.smid = syl.start_time + syl.duration / 2
		varctx["start"] = varctx.sstart
		varctx["end"] = varctx.send
		varctx.dur = varctx.sdur
		varctx.mid = varctx.smid
		varctx.si = syl.i
		varctx.i = varctx.si
		varctx.sleft = syl.left
		varctx.scenter = syl.center
		varctx.sright = syl.right
		if line.halign == "left" then
			varctx.sx = varctx.lleft + syl.left
		elseif line.halign == "center" then
			varctx.sx = varctx.lleft + syl.center
		elseif line.halign == "right" then
			varctx.sx = varctx.lleft + syl.right
		end
		varctx.sy = line.y
		varctx.left = varctx.sleft
		varctx.center = varctx.scenter
		varctx.right = varctx.sright
		varctx.x = varctx.sx
		varctx.y = varctx.sy
	end

	tenv.orgline = line
	tenv.line = nil
	tenv.syl = nil
	tenv.char = nil
	tenv.furi = nil

	-- Apply all line templates
	for t in matching_templates(templates.line, line) do
		if t.code then
			run_template_code(t, tenv)
		else
			applied_templates = true
			local newline = table.copy(line)
			tenv.line = newline
			newline.text = ""
			if t.pre ~= "" then
				newline.text = newline.text .. run_text_template(t.pre, tenv, varctx)
			end
			if t.t ~= "" then
				for i = 1, line.kara.n do
					local syl = line.kara[i]
					tenv.syl = syl
					set_ctx_syl(syl)
					newline.text = newline.text .. run_text_template(t.t, tenv, varctx)
					if t.addtext then
						if t.keeptags then
							newline.text = newline.text .. syl.text
						else
							newline.text = newline.text .. syl.text_stripped
						end
					end
				end
			else
				-- hmm, no main template for the line... put original text in
				newline.text = newline.text .. line.text
			end
			newline.effect = "fx"
			subs.append(newline)
		end
	end
	
	-- Loop over syllables
	for i = 0, line.kara.n do
		local syl = line.kara[i]
		
		-- Apply syllable templates
		for t in matching_templates(templates.syl, line) do
			tenv.syl = syl
			set_ctx_syl(syl)
	
			if not t.inline_fx or t.inline_fx == syl.inline_fx then
				if t.code then
					run_code_template(t, tenv)
				elseif t.multi then
					for hl = 1, syl.highlights.n do
						local hldata = syl.highlights[hl]
						local hlsyl = table.copy(syl)
						hlsyl.start_time = hldata.start_time
						hlsyl.end_time = hldata.end_time
						hlsyl.duration = hldata.duration
						tenv.basesyl = syl
						tenv.syl = hlsyl
						set_ctx_syl(hlsyl)
						
						for j = 1, t.loops do
							tenv.j = j
							local newline = table.copy(line)
							tenv.line = newline
							newline.text = run_text_template(t, tenv, varctx)
							if t.addtext then
								newline.text = newline.text .. syl.text_stripped
							end
							newline.effect = "fx"
							subs.append(newline)
						end
					end
				else
					for j = 1, t.loops do
						tenv.j = j
						local newline = table.copy(line)
						tenv.line = newline
						newline.text = run_text_template(t, tenv, varctx)
						if t.addtext then
							newline.text = newline.text .. syl.text_stripped
						end
						newline.effect = "fx"
						subs.append(newline)
					end
				end
			end
		end
		
		-- Apply character templates
		-- TODO
	end
	
	-- Loop over furigana
	for i = 1, line.furi.n do
		-- TODO
	end
end

function run_code_template(template, tenv)
	local f, err = loadstring(template.code, "template code")
	if not f then
		aegisub.debug.out(2, "Failed to parse Lua code: %s\nCode that failed to parse: %s\n\n", err, template.code)
	else
		setfenv(f, tenv)
		for j = 1, template.loops do
			tenv.j = j
			local res, err = pcall(f)
			if not res then
				aegisub.debug.out(2, "Runtime error in template code: %s\nCode producing error: %s\n\n", err, template.code)
			end
		end
	end
end

function run_text_template(template, tenv, varctx)
	local res = template
	
	-- Replace the variables in the string (this is probably faster than using a custom function, but doesn't provide error reporting)
	if varctx then
		res = string.gsub(res, "$([%a_]+)", varctx)
	end
	
	-- Function for evaluating expressions
	local function expression_evaluator(expression)
		f, err = loadstring(string.format("return (%s)", expression))
		if (err) ~= nil then
			aegisub.debug.out(2, "Error parsing expression: %s\nExpression producing error: %s\nTemplate with expression: %s\n\n", err, expression, template)
			return "!" .. expression .. "!"
		else
			setfenv(f, tenv)
			local res, val = pcall(f)
			if res then
				return val
			else
				aegisub.debug.out(2, "Runtime error in template expression: %s\nExpression producing error: %s\nTemplate with expression: %s\n\n", val, expression, template)
				return "!" .. expression .. "!"
			end
		end
	end
	-- Find and evaluate expressions
	res = string.gsub(res , "!(.-)!", expression_evaluator)
	
	return res
end


-- Main function to do the templating
function filter_apply_templates(subs, config)
	aegisub.progress.task("Collecting header data...")
	local meta, styles = karaskel.collect_head(subs)
	
	aegisub.progress.task("Parsing templates...")
	local templates = parse_templates(meta, styles, subs)
	
	aegisub.progress.task("Applying templates...")
	apply_templates(meta, styles, subs, templates)
end

function macro_apply_templates(subs, sel)
	filter_apply_templates(subs, {ismacro=true, sel=sel})
	aegisub.set_undo_point("apply karaoke template")
end

function macro_can_template(subs)
	-- check if this file has templates in it, don't allow running the macro if it hasn't
	local num_dia = 0
	for i = 1, #subs do
		local l = subs[i]
		if l.class == "dialogue" then
			num_dia = num_dia + 1
			-- test if the line is a template
			if (string.headtail(l.effect)):lower() == "template" then
				return true
			end
			-- don't try forever, this has to be fast
			if num_dia > 50 then
				return false
			end
		end
	end
	return false
end

aegisub.register_macro("Apply karaoke template", "Applies karaoke effects from templates", macro_apply_templates, macro_can_template)
aegisub.register_filter("Karaoke temokate", "Apply karaoke effect templates to the subtitles", 2000, filter_apply_templates)
