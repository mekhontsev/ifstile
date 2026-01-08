var cacheName = 'ifstile';
var filesToCache = [
	'./',
	'./index.html',
	'./${NAME_VER}.js',
	'./${NAME_VER}.wasm',
	'./manifest.json',
	'./ifstile192mask.png',
	'./ifstile512.png',
	'./apple-touch-icon.png',
	'./favicon.ico'
];


self.addEventListener('install', event => {
	event.waitUntil(
		caches.open(cacheName).then(cache => {cache.addAll(filesToCache);
	}));
});


self.addEventListener('fetch', event => {
	event.respondWith(caches.match(event.request)
		.then(response => {return response || fetch(event.request);})
	);
});