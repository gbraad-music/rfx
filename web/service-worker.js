const CACHE_NAME = 'rfx-effects-v62';
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
    './wasm-mappings.json',
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

// Fetch event - CACHE FIRST (offline-first, reliable on flaky networks)
self.addEventListener('fetch', (event) => {
    // Skip caching external resources
    if (!event.request.url.startsWith(self.location.origin)) {
        return;
    }

    event.respondWith(
        // Try cache FIRST for instant response
        caches.match(event.request).then((cachedResponse) => {
            if (cachedResponse) {
                // Serve from cache immediately (offline-first)
                console.log('[ServiceWorker] Serving from cache:', event.request.url);

                // Update cache in background (stale-while-revalidate)
                fetch(event.request).then((networkResponse) => {
                    if (networkResponse && networkResponse.status === 200) {
                        caches.open(CACHE_NAME).then((cache) => {
                            cache.put(event.request, networkResponse.clone());
                        });
                    }
                }).catch(() => {
                    // Network failed, but we already served from cache, so ignore
                });

                return cachedResponse;
            }

            // Not in cache, try network
            return fetch(event.request).then((networkResponse) => {
                if (networkResponse && networkResponse.status === 200) {
                    const responseToCache = networkResponse.clone();
                    caches.open(CACHE_NAME).then((cache) => {
                        cache.put(event.request, responseToCache);
                    });
                }
                return networkResponse;
            }).catch(() => {
                // Network failed and not in cache - offline fallback
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
