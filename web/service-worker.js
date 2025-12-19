const CACHE_NAME = 'rfx-effects-v54';
const ASSETS = [
    './',
    './index.html',
    './manifest.json',
    './icon.svg',
    './icon-192.png',
    './icon-512.png',
    './effects.js',
    './audio-worklet-processor.js',
    './regroove-effects.js',
    './regroove-effects.wasm',
    './pad-knob.js',
    './svg-slider.js',
    './fader-components.js'
];

// Install event - cache assets
self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME).then((cache) => {
            console.log('[ServiceWorker] Caching app assets');
            // Cache files individually to avoid failure if one file is missing
            return Promise.allSettled(
                ASSETS.map(url =>
                    cache.add(url).catch(err => {
                        console.warn('[ServiceWorker] Failed to cache:', url, err.message);
                    })
                )
            );
        })
    );
    self.skipWaiting();
});

// Activate event - clean up old caches
self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((cacheNames) => {
            return Promise.all(
                cacheNames.map((cacheName) => {
                    if (cacheName !== CACHE_NAME) {
                        console.log('[ServiceWorker] Deleting old cache:', cacheName);
                        return caches.delete(cacheName);
                    }
                })
            );
        })
    );
    self.clients.claim();
});

// Fetch event - NETWORK FIRST (always check for updates)
self.addEventListener('fetch', (event) => {
    // Skip caching external resources
    if (!event.request.url.startsWith(self.location.origin)) {
        return;
    }

    event.respondWith(
        // Try network first, fallback to cache
        fetch(event.request).then((networkResponse) => {
            // Clone the response immediately before it's consumed
            // Only cache valid responses (not opaque or error responses)
            if (networkResponse && networkResponse.status === 200) {
                const responseToCache = networkResponse.clone();
                // Cache asynchronously (don't block the response)
                caches.open(CACHE_NAME).then((cache) => {
                    cache.put(event.request, responseToCache);
                }).catch(err => {
                    console.warn('[ServiceWorker] Failed to cache:', event.request.url, err.message);
                });
            }
            return networkResponse;
        }).catch(() => {
            // Network failed, try cache
            return caches.match(event.request).then((cachedResponse) => {
                if (cachedResponse) {
                    return cachedResponse;
                }
                // No cache either - offline fallback
                if (event.request.destination === 'document') {
                    return caches.match('./index.html');
                }
                return new Response('Offline', {
                    status: 503,
                    statusText: 'Service Unavailable',
                    headers: new Headers({
                        'Content-Type': 'text/plain'
                    })
                });
            });
        })
    );
});
