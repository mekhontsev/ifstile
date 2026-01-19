// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"
#include "conbuf.h"
#include "version.h"
#include "call_thread.h"
#include "platform.h"
#include "ims_worker.h"

#include "imgui_impl_sdl3.h"


#if defined(IMS_USE_DX11)
#include "imgui_impl_dx11.h"
#include "dx_helper.h"
#elif defined(IMS_USE_GL3)
#include "imgui_impl_opengl3.h"
#else
#include "imgui_impl_opengl2.h"
#endif


ims_static SDL_Window* g_window = nullptr;
ims_static bool g_embedded_mode = false;

void try_open_file(std::function<void()>&& F, bool use_confirm);
bool on_draw();
void init_resolution(int w, int h, float scale);
void on_start();
void on_exit();
void save_settings();
void do_exit_confirm();
void set_app_icon();
void ims_num_traits_init_all();
float* get_background_data_rgb();
void ims_on_key_down(const SDL_KeyboardEvent&);


namespace platform{
	std::string_view getPathPref();
}

#if defined(IMS_USE_DX11)

ims_static dx11_helper g_dx11;

dx11_helper& get_d3d_device()
{
	return g_dx11;
}

#endif




////////////////////////////////////////////////////////////////////////////////

struct ims_event_queue
{
	struct event
	{
		using light_msg_type = void (*)();

		void* full_msg;
		light_msg_type light_msg;

		void call() const
		{
			if (full_msg) {
				main_thread::dispatch(full_msg);
			}
			if (light_msg) {
				light_msg();
			}
		}
	};

	//can be called from different threads
	void push(void* p1, void* p2)
	{
		bool need_event;
		{
			std::scoped_lock lock(m_lock);
			need_event = m_data.empty();
			m_data.emplace_back(p1, (event::light_msg_type)p2);
		}
		if (!need_event)return;

		SDL_Event e;
		e.type = SDL_EVENT_USER;
		[[maybe_unused]]
		let res = SDL_PushEvent(&e);
		assert(res);
	};

	void process() 
	{
		{
			std::scoped_lock lock(m_lock);
			m_temp = m_data;//it's important that it's copied and not moved
			m_data.clear();
		}
		for (auto& q : m_temp) {
			q.call();
		}
	}

	bool empty() const { return m_data.empty(); }

private:

	std::mutex m_lock;
	std::vector<event> m_data;
	std::vector<event> m_temp;
};

ims_static ims_event_queue s_events;

void ext_async_message(void* p)
{
	s_events.push(p, nullptr);
};

void call_main_thread(void(*f)())
{
	s_events.push(nullptr, (void*)f);
};

////////////////////////////////////////////////////////////////////////////////

void update_ui_async()
{
#if !defined(__EMSCRIPTEN__)
	call_main_thread(nullptr);
#endif // !__EMSCRIPTEN__
};

void redraw_gui(unsigned)
{
	//	s_extra_iters = std::max(s_extra_iters, v);
	call_main_thread(nullptr);
}


template <typename... T>
void show_startup_error(const char* title,
	fmt::format_string<T...> fmt, 
	T&&... args) 
{
	fmt::vargs<T...> vargs = { {args...} };
	std::ostringstream ss;
	fmt::vprint(ss, fmt.str, vargs);
	let msg = ss.str();

	//////////////////////////////////////
	SDL_Log("%s: %s", title, msg.c_str());
	platform::message(msg.c_str(), title);
}

static void show_gl_error_once() 
{
	static bool gl_err_checked = false;
	if (gl_err_checked)return;
	gl_err_checked = true;

#ifdef IMS_USE_DX11
	
#else
	let ge = glGetError();
	if (ge == GL_NO_ERROR)return;
	show_startup_error("OpenGL Error", "#{}", ge);
#endif
}

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
static bool HandleAppEvents(void*, SDL_Event* event)
{
	switch (event->type)
	{
	case SDL_EVENT_TERMINATING:
		/* Terminate the app.
		   Shut everything down before returning from this function.
		*/
		save_settings();
		return false;
	case SDL_EVENT_LOW_MEMORY:
		/* You will get this when your app is paused and iOS wants more memory.
		   Release as much memory as possible.
		*/
		return false;
	case SDL_EVENT_WILL_ENTER_BACKGROUND:
		/* Prepare your app to go into the background.  Stop loops, etc.
		   This gets called when the user hits the home button, or gets a call.
		*/
		ims_worker::pause_workers(true);
		return false;
	case SDL_EVENT_DID_ENTER_BACKGROUND:
		/* This will get called if the user accepted whatever sent your app to the background.
		   If the user got a phone call and canceled it, you'll instead get an    SDL_APP_DIDENTERFOREGROUND event and restart your loops.
		   When you get this, you have 5 seconds to save all your state or the app will be terminated.
		   Your app is NOT active at this point.
		*/
		return false;
	case SDL_EVENT_WILL_ENTER_FOREGROUND:
		/* This call happens when your app is coming back to the foreground.
			Restore all your state here.
		*/
		return false;
	case SDL_EVENT_DID_ENTER_FOREGROUND:
		/* Restart your loops here.
		   Your app is interactive and getting CPU again.
		*/
		ims_worker::pause_workers(false);
		return false;
	default:
		/* No special processing, add it to the event queue */
		return true;
	}
}


//the body will be executed only once
static void set_event_filter_once()
{
	static bool s_ready = false;
	if (s_ready)return;
	s_ready = true;
	//removes all pending events from the queue. Therefore, if you do this before
	//they are processed, they will be lost.
	//In particular, on Android, SDL_DROPFILE gets lost and
	//Open With... stops working.
	SDL_SetEventFilter(HandleAppEvents, nullptr);
};
#endif



////////////////////////////////////////////////////////////////////////////////
#if !defined(__EMSCRIPTEN__)
static const uint32_t c_ui_timer_ms = 100;
#endif

void timer_callback(size_t ms_interval);//1 time per second, update performance counters
bool timer_callback100ms(size_t ms_interval);//10 times per second

bool is_program_minimized();
void ext_prevent_sleep_mode(bool prevent);

static bool slow_checker()
{
	static bool active = false;

	let aw = ims_worker::active_workers();

	if (!active && aw > 0) {
		active = true;
		ext_prevent_sleep_mode(true);
		return true;
	}
	
	if (active && aw == 0) {
		active = false;
		ext_prevent_sleep_mode(false);
		return true;
	}
	
	return aw > 0 && !is_program_minimized();
	
}


void open_file(
	const std::string& filename,
	std::string&& data,//content (may be empty)
	bool keep_set,
	bool to_recent,
	const struct edit_info* edit);
////////////////////////////////////////////////////////////////////////////////

ims_static console_writer g_conbuf;

ims_static unsigned s_extra_iters = 0;


//file name to open
ims_static std::string s_file_name_to_open;
//file contents: if empty, then read from the file system
ims_static std::string s_file_data_to_open;


static void set_open_file(
	std::string_view fname, 
	std::string_view content,
	bool direct)
{	
	while (!fname.empty() && fname.front() == ' ') {
		fname.remove_prefix(1);
	}

	if (fname.empty() && content.empty())return;

#ifdef __APPLE__ 
	static constexpr std::string_view pre = "-psn";
	if (fname.starts_with(pre)) {//unique identifier for an open process.
		return;
	}
#endif

	if (direct) {
		s_file_name_to_open = fname;
		s_file_data_to_open = content;
	} else {
		std::string fn(fname);
		std::string fc(content);
		try_open_file([fn = std::move(fn), fc = std::move(fc)]{
			s_file_name_to_open = std::move(fn);
			s_file_data_to_open = std::move(fc);
		}, true);
	}
}

static void handle_file() 
{
	if (s_file_name_to_open.empty() && s_file_data_to_open.empty())return;
	
	open_file(
		s_file_name_to_open, 
		std::move(s_file_data_to_open), 
		false, true, nullptr);

	s_file_name_to_open.clear();
	s_file_data_to_open.clear();
}




#ifdef __EMSCRIPTEN__

void  (*g_ImGuiSetClipboardTextFn)(ImGuiContext* user_data, const char* text) = nullptr;

static void set_imgui_private_clipboard_text(const char* text) 
{      
	assert(g_ImGuiSetClipboardTextFn);
	if (g_ImGuiSetClipboardTextFn) {
		//The first parameter is implementation-specific!
		g_ImGuiSetClipboardTextFn(ImGui::GetCurrentContext(), text);
	}
}

extern "C" {

	//exported to JS
	EMSCRIPTEN_KEEPALIVE void browser_back();
	EMSCRIPTEN_KEEPALIVE void file_open(const char* filename, const char* data, int size);
	EMSCRIPTEN_KEEPALIVE void string_open(const char* contents);
	
	//callbacks
	void one_step();
	EM_BOOL visibilitychange_callback(
		int, const EmscriptenVisibilityChangeEvent* e, void*);
}


static void enable_keyboard_events(bool en)
{
	
	SDL_SetEventEnabled(SDL_EVENT_TEXT_INPUT, en);
	SDL_SetEventEnabled(SDL_EVENT_KEY_DOWN, en);

	//Otherwise, ImGui won't know that CTRL-V is no longer pressed and will spam the paste event
	//SDL_SetEventEnabled(SDL_EVENT_KEY_UP, en);

	//SDL_SetEventEnabled(SDL_EVENT_TEXT_EDITING, en);//It's unclear whether this is necessary
};


void browser_back() 
{
	void on_back_button_pressed();
	on_back_button_pressed();
};

//EMSCRIPTEN: can be called before main()
void file_open(const char* filename, const char* data, int size ) {
#if defined(DEVELOPER_VERSION)
	ims_print("file_open: {} {} {} {}\n", 
		filename, size, (int)data[0], (int)data[size - 1]);
#endif
	if (!g_window) {
		g_embedded_mode = true;
	}
	set_open_file(filename, std::string_view(data,size), !g_window);
};

void string_open(const char* contents)
{
#if defined(DEVELOPER_VERSION)
	ims_print("string_open: {}\n", contents);
#endif

	enable_keyboard_events(true);

	extern std::string Emscripten_paste_buffer;
	Emscripten_paste_buffer = contents;
	
	set_imgui_private_clipboard_text(contents);
};


void Emscripten_switch_to_editor()
{
#if defined(DEVELOPER_VERSION)
	ims_print("switch_to_editor called\n");
#endif

	EM_ASM_({ Module.dedit(); });

	//allow editing
	//will definitely call string_open later and restore
	enable_keyboard_events(false);
};

static bool is_standalone_mode()
{
	return 0 < EM_ASM_INT(
		return window.matchMedia('(display-mode: standalone)').matches ? 1 : 0
	);
}

EM_BOOL visibilitychange_callback(int, const EmscriptenVisibilityChangeEvent* e, void*)
{
	if(e->hidden){
		save_settings();
	}

#if defined(DEVELOPER_VERSION)
	ims_print("visibilitychange_callback = {}\n", e->hidden);
#endif

	return EM_TRUE;
}


#endif //__EMSCRIPTEN__



SDL_Window* MainWindow_get()
{
	return g_window;
}

bool is_embedded_mode()
{
	return g_embedded_mode;
}

bool is_program_minimized()
{
	return (SDL_GetWindowFlags(MainWindow_get()) & SDL_WINDOW_MINIMIZED) != 0;
}


void set_window_title(const char* t)
{ 
#if defined(__EMSCRIPTEN__)
	if (!is_standalone_mode())return;
#endif
	SDL_SetWindowTitle(MainWindow_get(), t);

}

void switch_fullscreen()
{
	auto* w = MainWindow_get();
	let is_fullscreen = SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN;
	SDL_SetWindowFullscreen(w, is_fullscreen == 0);
};


std::string get_con_data(bool err = false)
{
	return err ? g_conbuf.fetch_error() : g_conbuf.fetch_string();
};


//when the fast and slow update routines were last called
ims_static ims_chrono g_time_slow;

#if !defined(__EMSCRIPTEN__)
ims_static ims_chrono  g_time_fast;
#endif


static void draw_frame()
{
	
#ifdef _MSC_VER
	//protection against recursive calls (when changing dark<->light on Windows)
	static bool draw_in_progress = false;
	if (draw_in_progress) {
		return;
	}
	draw_in_progress = true;
	IMS_SCOPE([] {draw_in_progress=false; });
#endif

#if defined(IMS_USE_DX11)
	ImGui_ImplDX11_NewFrame();
#elif defined(IMS_USE_GL3)
	ImGui_ImplOpenGL3_NewFrame();
#else
	ImGui_ImplOpenGL2_NewFrame();
#endif
	
	ImGui_ImplSDL3_NewFrame();
	
	ImGui::NewFrame();

	show_gl_error_once();

	////////////////////////////////////////////////////////////////////////////
	//what accumulated in the previous iteration is processed at the beginning of the current one
	s_events.process();
	
	////////////////////////////////////////////////////////////////////////////
	
	//rendering the entire GUI...
	
	if (on_draw()) {

		let* b = get_background_data_rgb();

		

#ifdef IMS_USE_DX11
		g_dx11.ClearColor({ b[0], b[1], b[2], 1.0f });
#else
		glClearColor(b[0], b[1], b[2], 1);
		glClear(GL_COLOR_BUFFER_BIT);
#endif

		//after on_draw, to ensure a non-empty workspace size is defined for
		//plotting. Otherwise, if the CONTENTS (base64)
		//file is specified on the command line, plotting will not start when opened via a URL, etc.
		handle_file();
	}

	////////////////////////////////////////////////////////////////////////////

	ImGui::Render();

#if defined(IMS_USE_DX11)
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	g_dx11.pSwapChain->Present(1, 0); // Present with vsync
	//g_dx11.g_pSwapChain->Present(0, 0); // Present without vsync
#elif defined(IMS_USE_GL3)
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(g_window);
#else
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(g_window);
#endif

	
};


#ifdef __EMSCRIPTEN__
#define main_scope(body)
#else 
#define main_scope(body) IMS_SCOPE(body);
#endif

void one_step()
{
	bool need_draw = true;

	if (!s_events.empty()) {
		s_extra_iters = 1;
	}

	if (s_extra_iters > 0) {
		--s_extra_iters;
	} else {
#if !defined (__EMSCRIPTEN__)
		need_draw = SDL_WaitEventTimeout(nullptr, c_ui_timer_ms);
#endif //__EMSCRIPTEN__
	}

	////////////////////////////////////////////////////////////////////////
	let cur_time = ims_chrono::now();
	
#if !defined(__EMSCRIPTEN__)
	let dfast =	ims_chrono::dif_ms(g_time_fast, cur_time);
	if (dfast >= c_ui_timer_ms) {
		g_time_fast = cur_time;
		if (!need_draw) {
			need_draw = timer_callback100ms(dfast);
		}
	}
#endif //__EMSCRIPTEN__

	let dslow = ims_chrono::dif_ms(g_time_slow, cur_time);
	if (dslow >= 1000) {
		if (!need_draw) {
			need_draw = slow_checker();
		}
		g_time_slow = cur_time;
		timer_callback(dslow);//called constantly, but rarely
	}

	if (!need_draw) {
#if defined(__EMSCRIPTEN__)	
		assert(false);
#endif //__EMSCRIPTEN__
		return;
	}
	//from this point on the function should work to the end
	////////////////////////////////////////////////////////////////////////

	SDL_Event e;
	while (SDL_PollEvent(&e)) {//process all events

		s_extra_iters = 1;//One extra iteration is often not enough

		if (e.type == SDL_EVENT_USER) {
			
		} else {

			ImGui_ImplSDL3_ProcessEvent(&e);

			if(e.type == SDL_EVENT_KEY_DOWN){
				ims_on_key_down(e.key);
				
				//virtual keyboard "backspace" workaround
				//break;//looks like it's no longer needed
			}else if (e.type == SDL_EVENT_DROP_FILE) {
				//SDL_Log("SDL_DROPFILE = %s\n", e.drop.data);
				//ims_print("SDL_DROPFILE = {}\n", e.drop.data);
				set_open_file(e.drop.data, "", false);
			} else if (e.type == SDL_EVENT_QUIT) {
				do_exit_confirm();
			} else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				break;//touch workaround
			}
		}
	}

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
	set_event_filter_once();
#endif

	draw_frame();
};


//can be called from outside the GUI thread!
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
static bool eventWatcher(void*, SDL_Event* e)
{
	if (e->type == SDL_EVENT_WINDOW_RESIZED)
	{
		assert(ims_worker::is_main_thread());//just interesting

#if defined(IMS_USE_DX11)		
		g_dx11.OnResize();
#endif
		draw_frame();
		return false;
	}
	return true;
}
#endif

#if defined(DEVELOPER_VERSION)
static std::string QueryCapability()
{
#ifdef IMS_USE_DX11
	return "\n";
#else
	auto c = [](GLenum e) {
		return reinterpret_cast<const char*>(glGetString(e));
	};

	return std::string() +
		"GL_VENDOR = "		+ c(GL_VENDOR)		+ "\n" +
		"GL_RENDERER = "	+ c(GL_RENDERER)	+ "\n" +
		"GL_VERSION = "		+ c(GL_VERSION)		+ "\n" +
		"GL_EXTENSIONS = "	+ c(GL_EXTENSIONS)	+ "\n";
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////
int main_utf8(int argc, char **argv)
{
	////////////////////////////////////////////////////////////////////////////
	ims_num_traits_init_all();
	////////////////////////////////////////////////////////////////////////////

#ifdef TEST_ALLOC_HOOK_READY
	test_alloc_hook::init();
#endif

#ifdef __ANDROID__
	main_scope ([] {exit(0);});
#endif


	//first and foremost
	g_conbuf.redirect();
	main_scope([] { g_conbuf.revert(); });

	SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");	

	//SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");//default

	//SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
	//SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");

	
	ims_worker::init_main();
	
	////////////////////////////////////////////////////////////////////////////
	// Setup SDL
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		show_startup_error("Init", "{}", SDL_GetError());
		return -1;
	}


	main_scope ([] {SDL_Quit();});

	////////////////////////////////////////////////////////////////////////////
#if !defined(IMS_USE_DX11)
	// Setup window
	const int attributes[][2] =
	{
		{ SDL_GL_DOUBLEBUFFER,			1 },
		{ SDL_GL_DEPTH_SIZE,			24 },
		{ SDL_GL_STENCIL_SIZE,			8 },
		
#if defined(IMGUI_IMPL_OPENGL_ES3)
		{ SDL_GL_CONTEXT_MAJOR_VERSION, 3 },
		{ SDL_GL_CONTEXT_MINOR_VERSION, 0},
#else
		{ SDL_GL_CONTEXT_MAJOR_VERSION, 2 },
		{ SDL_GL_CONTEXT_MINOR_VERSION, 2 },
#endif

#if defined(IMGUI_IMPL_OPENGL_ES3) || defined(IMGUI_IMPL_OPENGL_ES2)
		{ SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES },
#endif


	};

	for (let* a : attributes) {
		let res = SDL_GL_SetAttribute((SDL_GLAttr)a[0], a[1]);
		if (!res) {
			show_startup_error("SetAttribute", "#{}, {}:{}", 
				SDL_GetError(), a[0], a[1]);
		}
	};
#endif

	////////////////////////////////////////////////////////////////////////////
	
	//creating an SDL window
	let displayID = SDL_GetPrimaryDisplay();

	auto* dm = SDL_GetCurrentDisplayMode(displayID);
	
	if (!dm) {
		show_startup_error("GetCurrentDisplayMode", 
			" {}, #{}", SDL_GetError(), displayID);
		return -1;
	}

	ims_print("Preferences path = {}\n", platform::getPathPref());


#if defined(DEVELOPER_VERSION)
	ims_print("SDL_GetCurrentDisplayMode = {}x{}\n", dm->w, dm->h);
#endif

	Uint32 flags = 
		SDL_WINDOW_HIGH_PIXEL_DENSITY | 
		SDL_WINDOW_RESIZABLE 
#if !defined(IMS_USE_DX11)
		| SDL_WINDOW_OPENGL 
#endif
		;

#ifdef _MSC_VER
	flags |= SDL_WINDOW_HIDDEN;//so that it doesn't flicker when the dark theme is enabled
#endif

	g_window = SDL_CreateWindow(APPLICATION_TITLE, dm->w / 2, dm->h / 2, flags);
	
	if (!g_window) {
		show_startup_error("CreateWindow", "{}", SDL_GetError());
		return -1;
	}

	main_scope ([] {
		SDL_DestroyWindow(g_window); g_window = nullptr;
	});


#if !defined(DEVELOPER_VERSION) || !defined(_MSC_VER)
	set_app_icon();
#endif

	////////////////////////////////////////////////////////////////////////////
	//context initialization
#if defined(IMS_USE_DX11)
	HWND SDLWindows_getHWND(SDL_Window * sdlWindow);
	auto hwnd = SDLWindows_getHWND(g_window);

	main_scope([] {g_dx11.CleanupDeviceD3D(); });

	let res = g_dx11.CreateDeviceD3D(hwnd);
	if (res != S_OK) {
		show_startup_error("DeviceD3D", "{}", HRESULT_CODE(res));
		return -1;
	}
#else
	auto* glcontext = SDL_GL_CreateContext(g_window);
	if (!glcontext) {
		show_startup_error("CreateContext", "{}", SDL_GetError());
		return -1;
	}

	main_scope ([&] {SDL_GL_DestroyContext(glcontext);});
#endif

	
#if defined(DEVELOPER_VERSION)
	ims_print("QueryCapability: {}", QueryCapability());
#endif


	////////////////////////////////////////////////////////////////////////////
	//initialize ImGui context
	let ini_path = std::string(platform::getPathPref()) + "imgui.ini";//before DestroyContext
	ImGui::CreateContext();
	main_scope([] {ImGui::DestroyContext();});


	ImGui::GetIO().IniFilename = ini_path.c_str();//does not own
	////////////////////////////////////////////////////////////////////////////
	// Setup ImGui GL binding
#if defined(IMS_USE_DX11)
	ImGui_ImplSDL3_InitForD3D(g_window);
#else
	ImGui_ImplSDL3_InitForOpenGL(g_window, glcontext);
#endif
	main_scope([] {ImGui_ImplSDL3_Shutdown(); });


#if defined(IMS_USE_DX11)
	ImGui_ImplDX11_Init(g_dx11.pd3dDevice, g_dx11.pd3dDeviceContext);
	main_scope([] {ImGui_ImplDX11_Shutdown(); });
#elif defined(IMS_USE_GL3)
	ImGui_ImplOpenGL3_Init();
	main_scope([]{ImGui_ImplOpenGL3_Shutdown();});
#else
	ImGui_ImplOpenGL2_Init();
	main_scope([] {ImGui_ImplOpenGL2_Shutdown(); });
#endif

	////////////////////////////////////////////////////////////////////////////

	int width, height;
	SDL_GetWindowSizeInPixels(g_window, &width, &height);
	const float scale = SDL_GetWindowDisplayScale(g_window);

#if defined(DEVELOPER_VERSION)
	ims_print("SDL_GetWindowSizeInPixels = {}x{}\n", width, height);
	ims_print("SDL_GetWindowDisplayScale = {}\n", scale);
#endif

#ifdef __EMSCRIPTEN__

#if 0
	ImGui::GetPlatformIO().Platform_GetClipboardTextFn = [](ImGuiContext*) ->const char* {
		bool ims_from_clipboard(std::string& dst);
		static std::string str;
		if (!ims_from_clipboard(str)) {
			return "";//this will show a text box
		}
		return str.c_str();
	};
#endif

	g_ImGuiSetClipboardTextFn = ImGui::GetPlatformIO().Platform_SetClipboardTextFn;
	ImGui::GetPlatformIO().Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text){
		set_imgui_private_clipboard_text(text);
		platform::ims_to_clipboard(text);
	};

#endif // __EMSCRIPTEN__

	

	init_resolution(width, height, scale);//before on_start
	on_start();//before parsing the command line!
	
	main_scope([] {
		save_settings();
		on_exit();
	});

	
	if (argc >= 2) {
		set_open_file(argv[1], "", true);
	} else {
		handle_file();
	}

	////////////////////////////////////////////////////////////////////////////

	let ct = ims_chrono::now();
	g_time_slow = ct;
#if !defined(__EMSCRIPTEN__)
	g_time_fast = ct;
#endif

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
	SDL_AddEventWatch(eventWatcher, nullptr);
#endif

#if defined(_MSC_VER)
	SDL_ShowWindow(g_window);//because the above was created hidden
#endif


#ifdef __EMSCRIPTEN__
	//for some reason, SDL_Unsupported() is called somewhere inside SDL_Init and others
	SDL_ClearError();
#endif

	let* sdl_err = SDL_GetError();
	if(*sdl_err != 0){
		std::cerr << sdl_err << std::endl;
		SDL_ClearError();
	}

#if defined(__EMSCRIPTEN__)

	emscripten_set_visibilitychange_callback(nullptr, false, 
		visibilitychange_callback);

	emscripten_set_main_loop(one_step, 0, false);
#else //!__EMSCRIPTEN__
	while (!ims_worker::is_exit_program()) {
		one_step();
	}
#endif //__EMSCRIPTEN__
	
	
	return 0;
}


